/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001,2002 Russell L. Smith.       *
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org          *
 *                                                                       *
 * This library is free software; you can redistribute it and/or         *
 * modify it under the terms of EITHER:                                  *
 *   (1) The GNU Lesser General Public License as published by the Free  *
 *       Software Foundation; either version 2.1 of the License, or (at  *
 *       your option) any later version. The text of the GNU Lesser      *
 *       General Public License is included with this library in the     *
 *       file LICENSE.TXT.                                               *
 *   (2) The BSD-style license that is included with this library in     *
 *       the file LICENSE-BSD.TXT.                                       *
 *                                                                       *
 * This library is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the files    *
 * LICENSE.TXT and LICENSE-BSD.TXT for more details.                     *
 *                                                                       *
 *************************************************************************/

/*

design note: the general principle for giving a joint the option of connecting
to the static environment (i.e. the absolute frame) is to check the second
body (joint->node[1].body), and if it is zero then behave as if its body
transform is the identity.

*/

#include <ode/ode.h>
#include <ode/rotation.h>
#include "../config.h"
#include "../matrix.h"
#include "../odemath.h"
#include "joint.h"
#include "joint_internal.h"
#include "../util.h"

extern void addObjectToList( dObject *obj, dObject **first );

dxJoint::dxJoint( dxWorld *w ) :
    dObject( w )
{
    //printf("constructing %p\n", this);
    dIASSERT( w );
    flags = 0;
    node[0].joint = this;
    node[0].body = 0;
    node[0].next = 0;
    node[1].joint = this;
    node[1].body = 0;
    node[1].next = 0;
    dSetZero( lambda, 6 );

    addObjectToList( this, ( dObject ** ) &w->firstjoint );

    w->nj++;
    feedback = 0;
}

dxJoint::~dxJoint()
{ }


/*virtual */
void dxJoint::setRelativeValues()
{
    // Do nothing
}

bool dxJoint::isEnabled() const
{
    return ( (flags & dJOINT_DISABLED) == 0 &&
        (node[0].body->invMass > 0 ||
        (node[1].body && node[1].body->invMass > 0)) );
}


sizeint dxJointGroup::exportJoints(dxJoint **jlist)
{
    sizeint i=0;
    dxJoint *j = (dxJoint*) m_stack.rewind();
    while (j != NULL) {
        jlist[i++] = j;
        j = (dxJoint*) (m_stack.next (j->size()));
    }
    return i;
}

void dxJointGroup::freeAll()
{
    m_num = 0;
    m_stack.freeAll();
}


//****************************************************************************
// externs

// extern "C" void dBodyAddTorque (dBodyID, dReal fx, dReal fy, dReal fz);
// extern "C" void dBodyAddForce (dBodyID, dReal fx, dReal fy, dReal fz);

//****************************************************************************
// utility

// set three "ball-and-socket" rows in the constraint equation, and the
// corresponding right hand side.

void setBall( dxJoint *joint, dReal fps, dReal erp,
    int rowskip, dReal *J1, dReal *J2,
    int pairskip, dReal *pairRhsCfm,
    dVector3 anchor1, dVector3 anchor2 )
{
    // anchor points in global coordinates with respect to body PORs.
    dVector3 a1, a2;

    // set Jacobian
    J1[dxJoint::GI2_JLX] = 1;
    J1[rowskip + dxJoint::GI2_JLY] = 1;
    J1[2 * rowskip + dxJoint::GI2_JLZ] = 1;
    dMultiply0_331( a1, joint->node[0].body->posr.R, anchor1 );
    dSetCrossMatrixMinus( J1 + dxJoint::GI2__JA_MIN, a1, rowskip );

    dxBody *b1 = joint->node[1].body;
    if ( b1 )
    {
        J2[dxJoint::GI2_JLX] = -1;
        J2[rowskip + dxJoint::GI2_JLY] = -1;
        J2[2 * rowskip + dxJoint::GI2_JLZ] = -1;
        dMultiply0_331( a2, b1->posr.R, anchor2 );
        dSetCrossMatrixPlus( J2 + dxJoint::GI2__JA_MIN, a2, rowskip );
    }

    // set right hand side
    dReal k = fps * erp;
    dxBody *b0 = joint->node[0].body;
    if ( b1 )
    {
        dReal *currRhsCfm = pairRhsCfm;
        for ( int j = dSA__MIN; j != dSA__MAX; j++ )
        {
            currRhsCfm[dxJoint::GI2_RHS] = k * ( a2[j] + b1->posr.pos[j] - a1[j] - b0->posr.pos[j] );
            currRhsCfm += pairskip;
        }
    }
    else
    {
        dReal *currRhsCfm = pairRhsCfm;
        for ( int j = dSA__MIN; j != dSA__MAX; j++ )
        {
            currRhsCfm[dxJoint::GI2_RHS] = k * ( anchor2[j] - a1[j] - b0->posr.pos[j] );
            currRhsCfm += pairskip;
        }
    }
}


// this is like setBall(), except that `axis' is a unit length vector
// (in global coordinates) that should be used for the first jacobian
// position row (the other two row vectors will be derived from this).
// `erp1' is the erp value to use along the axis.

void setBall2( dxJoint *joint, dReal fps, dReal erp,
    int rowskip, dReal *J1, dReal *J2,
    int pairskip, dReal *pairRhsCfm,
    dVector3 anchor1, dVector3 anchor2,
    dVector3 axis, dReal erp1 )
{
    // anchor points in global coordinates with respect to body PORs.
    dVector3 a1, a2;

    // get vectors normal to the axis. in setBall() axis,q1,q2 is [1 0 0],
    // [0 1 0] and [0 0 1], which makes everything much easier.
    dVector3 q1, q2;
    dPlaneSpace( axis, q1, q2 );

    // set Jacobian
    dCopyVector3(J1 + dxJoint::GI2__JL_MIN, axis);
    dCopyVector3(J1 + rowskip + dxJoint::GI2__JL_MIN, q1);
    dCopyVector3(J1 + 2 * rowskip + dxJoint::GI2__JL_MIN, q2);
    dMultiply0_331( a1, joint->node[0].body->posr.R, anchor1 );
    dCalcVectorCross3( J1 + dxJoint::GI2__JA_MIN, a1, axis );
    dCalcVectorCross3( J1 + rowskip + dxJoint::GI2__JA_MIN, a1, q1 );
    dCalcVectorCross3( J1 + 2 * rowskip + dxJoint::GI2__JA_MIN, a1, q2 );

    dxBody *b0 = joint->node[0].body;
    dAddVectors3(a1, a1, b0->posr.pos);

    // set right hand side - measure error along (axis,q1,q2)
    dReal k1 = fps * erp1;
    dReal k = fps * erp;

    dxBody *b1 = joint->node[1].body;
    if ( b1 )
    {
        dCopyNegatedVector3(J2 + dxJoint::GI2__JL_MIN, axis);
        dCopyNegatedVector3(J2 + rowskip + dxJoint::GI2__JL_MIN, q1);
        dCopyNegatedVector3(J2 + 2 * rowskip + dxJoint::GI2__JL_MIN, q2);
        dMultiply0_331( a2, b1->posr.R, anchor2 );
        dCalcVectorCross3( J2 + dxJoint::GI2__JA_MIN, axis, a2 ); //== dCalcVectorCross3( J2 + dxJoint::GI2__J2A_MIN, a2, axis ); dNegateVector3( J2 + dxJoint::GI2__J2A_MIN );
        dCalcVectorCross3( J2 + rowskip + dxJoint::GI2__JA_MIN, q1, a2 ); //== dCalcVectorCross3( J2 + rowskip + dxJoint::GI2__J2A_MIN, a2, q1 ); dNegateVector3( J2 + rowskip + dxJoint::GI2__J2A_MIN );
        dCalcVectorCross3( J2 + 2 * rowskip + dxJoint::GI2__JA_MIN, q2, a2 ); //== dCalcVectorCross3( J2 + 2 * rowskip + dxJoint::GI2__J2A_MIN, a2, q2 ); dNegateVector3( J2 + 2 * rowskip + dxJoint::GI2__J2A_MIN );

        dAddVectors3(a2, a2, b1->posr.pos);

        dVector3 a2_minus_a1;
        dSubtractVectors3(a2_minus_a1, a2, a1);
        pairRhsCfm[dxJoint::GI2_RHS] = k1 * dCalcVectorDot3( axis, a2_minus_a1 );
        pairRhsCfm[pairskip + dxJoint::GI2_RHS] = k * dCalcVectorDot3( q1, a2_minus_a1 );
        pairRhsCfm[2 * pairskip + dxJoint::GI2_RHS] = k * dCalcVectorDot3( q2, a2_minus_a1 );
    }
    else
    {
        dVector3 anchor2_minus_a1;
        dSubtractVectors3(anchor2_minus_a1, anchor2, a1);
        pairRhsCfm[dxJoint::GI2_RHS] = k1 * dCalcVectorDot3( axis, anchor2_minus_a1 );
        pairRhsCfm[pairskip + dxJoint::GI2_RHS] = k * dCalcVectorDot3( q1, anchor2_minus_a1 );
        pairRhsCfm[2 * pairskip + dxJoint::GI2_RHS] = k * dCalcVectorDot3( q2, anchor2_minus_a1 );
    }
}


// set three orientation rows in the constraint equation, and the
// corresponding right hand side.

void setFixedOrientation( dxJoint *joint, dReal fps, dReal erp,
    int rowskip, dReal *J1, dReal *J2,
    int pairskip, dReal *pairRhsCfm,
    dQuaternion qrel )
{
    // 3 rows to make body rotations equal
    J1[dxJoint::GI2_JAX] = 1;
    J1[rowskip + dxJoint::GI2_JAY] = 1;
    J1[2 * rowskip + dxJoint::GI2_JAZ] = 1;

    dxBody *b1 = joint->node[1].body;
    if ( b1 )
    {
        J2[dxJoint::GI2_JAX] = -1;
        J2[rowskip + dxJoint::GI2_JAY] = -1;
        J2[2 * rowskip + dxJoint::GI2_JAZ] = -1;
    }

    // compute the right hand side. the first three elements will result in
    // relative angular velocity of the two bodies - this is set to bring them
    // back into alignment. the correcting angular velocity is
    //   |angular_velocity| = angle/time = erp*theta / stepsize
    //                      = (erp*fps) * theta
    //    angular_velocity  = |angular_velocity| * u
    //                      = (erp*fps) * theta * u
    // where rotation along unit length axis u by theta brings body 2's frame
    // to qrel with respect to body 1's frame. using a small angle approximation
    // for sin(), this gives
    //    angular_velocity  = (erp*fps) * 2 * v
    // where the quaternion of the relative rotation between the two bodies is
    //    q = [cos(theta/2) sin(theta/2)*u] = [s v]

    // get qerr = relative rotation (rotation error) between two bodies
    dQuaternion qerr, e;
    dxBody *b0 = joint->node[0].body;
    if ( b1 )
    {
        dQuaternion qq;
        dQMultiply1( qq, b0->q, b1->q );
        dQMultiply2( qerr, qq, qrel );
    }
    else
    {
        dQMultiply3( qerr, b0->q, qrel );
    }
    if ( qerr[0] < 0 )
    {
        qerr[1] = -qerr[1];  // adjust sign of qerr to make theta small
        qerr[2] = -qerr[2];
        qerr[3] = -qerr[3];
    }
    dMultiply0_331( e, b0->posr.R, qerr + 1 );  // @@@ bad SIMD padding!
    dReal k_mul_2 = fps * erp * REAL(2.0);
    pairRhsCfm[dxJoint::GI2_RHS] = k_mul_2 * e[dSA_X];
    pairRhsCfm[pairskip + dxJoint::GI2_RHS] = k_mul_2 * e[dSA_Y];
    pairRhsCfm[2 * pairskip + dxJoint::GI2_RHS] = k_mul_2 * e[dSA_Z];
}


// compute anchor points relative to bodies

void setAnchors( dxJoint *j, dReal x, dReal y, dReal z,
                dVector3 anchor1, dVector3 anchor2 )
{
    dxBody *b0 = j->node[0].body;
    if ( b0 )
    {
        dReal q[4];
        q[0] = x - b0->posr.pos[0];
        q[1] = y - b0->posr.pos[1];
        q[2] = z - b0->posr.pos[2];
        q[3] = 0;
        dMultiply1_331( anchor1, b0->posr.R, q );

        dxBody *b1 = j->node[1].body;
        if ( b1 )
        {
            q[0] = x - b1->posr.pos[0];
            q[1] = y - b1->posr.pos[1];
            q[2] = z - b1->posr.pos[2];
            q[3] = 0;
            dMultiply1_331( anchor2, b1->posr.R, q );
        }
        else
        {
            anchor2[0] = x;
            anchor2[1] = y;
            anchor2[2] = z;
        }
    }
    anchor1[3] = 0;
    anchor2[3] = 0;
}


// compute axes relative to bodies. either axis1 or axis2 can be 0.

void setAxes( dxJoint *j, dReal x, dReal y, dReal z,
             dVector3 axis1, dVector3 axis2 )
{
    dxBody *b0 = j->node[0].body;
    if ( b0 )
    {
        dReal q[4];
        q[0] = x;
        q[1] = y;
        q[2] = z;
        q[3] = 0;
        dNormalize3( q );

        if ( axis1 )
        {
            dMultiply1_331( axis1, b0->posr.R, q );
            axis1[3] = 0;
        }

        if ( axis2 )
        {
            dxBody *b1 = j->node[1].body;
            if ( b1 )
            {
                dMultiply1_331( axis2, b1->posr.R, q );
            }
            else
            {
                axis2[0] = x;
                axis2[1] = y;
                axis2[2] = z;
            }
            axis2[3] = 0;
        }
    }
}


void getAnchor( dxJoint *j, dVector3 result, dVector3 anchor1 )
{
    dxBody *b0 = j->node[0].body;
    if ( b0 )
    {
        dMultiply0_331( result, b0->posr.R, anchor1 );
        result[0] += b0->posr.pos[0];
        result[1] += b0->posr.pos[1];
        result[2] += b0->posr.pos[2];
    }
}


void getAnchor2( dxJoint *j, dVector3 result, dVector3 anchor2 )
{
    dxBody *b1 = j->node[1].body;
    if ( b1 )
    {
        dMultiply0_331( result, b1->posr.R, anchor2 );
        result[0] += b1->posr.pos[0];
        result[1] += b1->posr.pos[1];
        result[2] += b1->posr.pos[2];
    }
    else
    {
        result[0] = anchor2[0];
        result[1] = anchor2[1];
        result[2] = anchor2[2];
    }
}


void getAxis( dxJoint *j, dVector3 result, dVector3 axis1 )
{
    dxBody *b0 = j->node[0].body;
    if ( b0 )
    {
        dMultiply0_331( result, b0->posr.R, axis1 );
    }
}


void getAxis2( dxJoint *j, dVector3 result, dVector3 axis2 )
{
    dxBody *b1 = j->node[1].body;
    if ( b1 )
    {
        dMultiply0_331( result, b1->posr.R, axis2 );
    }
    else
    {
        result[0] = axis2[0];
        result[1] = axis2[1];
        result[2] = axis2[2];
    }
}


dReal getHingeAngleFromRelativeQuat( dQuaternion qrel, dVector3 axis )
{
    // the angle between the two bodies is extracted from the quaternion that
    // represents the relative rotation between them. recall that a quaternion
    // q is:
    //    [s,v] = [ cos(theta/2) , sin(theta/2) * u ]
    // where s is a scalar and v is a 3-vector. u is a unit length axis and
    // theta is a rotation along that axis. we can get theta/2 by:
    //    theta/2 = atan2 ( sin(theta/2) , cos(theta/2) )
    // but we can't get sin(theta/2) directly, only its absolute value, i.e.:
    //    |v| = |sin(theta/2)| * |u|
    //        = |sin(theta/2)|
    // using this value will have a strange effect. recall that there are two
    // quaternion representations of a given rotation, q and -q. typically as
    // a body rotates along the axis it will go through a complete cycle using
    // one representation and then the next cycle will use the other
    // representation. this corresponds to u pointing in the direction of the
    // hinge axis and then in the opposite direction. the result is that theta
    // will appear to go "backwards" every other cycle. here is a fix: if u
    // points "away" from the direction of the hinge (motor) axis (i.e. more
    // than 90 degrees) then use -q instead of q. this represents the same
    // rotation, but results in the cos(theta/2) value being sign inverted.

    // extract the angle from the quaternion. cost2 = cos(theta/2),
    // sint2 = |sin(theta/2)|
    dReal cost2 = qrel[0];
    dReal sint2 = dSqrt( qrel[1] * qrel[1] + qrel[2] * qrel[2] + qrel[3] * qrel[3] );
    dReal theta = ( dCalcVectorDot3( qrel + 1, axis ) >= 0 ) ? // @@@ padding assumptions
        ( 2 * dAtan2( sint2, cost2 ) ) :  // if u points in direction of axis
        ( 2 * dAtan2( sint2, -cost2 ) );  // if u points in opposite direction

    // the angle we get will be between 0..2*pi, but we want to return angles
    // between -pi..pi
    if ( theta > M_PI ) theta -= ( dReal )( 2 * M_PI );

    // the angle we've just extracted has the wrong sign
    theta = -theta;

    return theta;
}


// given two bodies (body1,body2), the hinge axis that they are connected by
// w.r.t. body1 (axis), and the initial relative orientation between them
// (q_initial), return the relative rotation angle. the initial relative
// orientation corresponds to an angle of zero. if body2 is 0 then measure the
// angle between body1 and the static frame.
//
// this will not return the correct angle if the bodies rotate along any axis
// other than the given hinge axis.

dReal getHingeAngle( dxBody *body1, dxBody *body2, dVector3 axis,
                    dQuaternion q_initial )
{
    // get qrel = relative rotation between the two bodies
    dQuaternion qrel;
    if ( body2 )
    {
        dQuaternion qq;
        dQMultiply1( qq, body1->q, body2->q );
        dQMultiply2( qrel, qq, q_initial );
    }
    else
    {
        // pretend body2->q is the identity
        dQMultiply3( qrel, body1->q, q_initial );
    }

    return getHingeAngleFromRelativeQuat( qrel, axis );
}

//****************************************************************************
// dxJointLimitMotor

void dxJointLimitMotor::init( dxWorld *world )
{
    vel = 0;
    fmax = 0;
    lostop = -dInfinity;
    histop = dInfinity;
    fudge_factor = 1;
    normal_cfm = world->global_cfm;
    stop_erp = world->global_erp;
    stop_cfm = world->global_cfm;
    bounce = 0;
    limit = 0;
    limit_err = 0;
}


void dxJointLimitMotor::set( int num, dReal value )
{
    switch ( num )
    {
    case dParamLoStop:
        lostop = value;
        break;
    case dParamHiStop:
        histop = value;
        break;
    case dParamVel:
        vel = value;
        break;
    case dParamFMax:
        if ( value >= 0 ) fmax = value;
        break;
    case dParamFudgeFactor:
        if ( value >= 0 && value <= 1 ) fudge_factor = value;
        break;
    case dParamBounce:
        bounce = value;
        break;
    case dParamCFM:
        normal_cfm = value;
        break;
    case dParamStopERP:
        stop_erp = value;
        break;
    case dParamStopCFM:
        stop_cfm = value;
        break;
    }
}


dReal dxJointLimitMotor::get( int num ) const
{
    switch ( num )
    {
    case dParamLoStop:
        return lostop;
    case dParamHiStop:
        return histop;
    case dParamVel:
        return vel;
    case dParamFMax:
        return fmax;
    case dParamFudgeFactor:
        return fudge_factor;
    case dParamBounce:
        return bounce;
    case dParamCFM:
        return normal_cfm;
    case dParamStopERP:
        return stop_erp;
    case dParamStopCFM:
        return stop_cfm;
    default:
        return 0;
    }
}


bool dxJointLimitMotor::testRotationalLimit( dReal angle )
{
    if ( angle <= lostop )
    {
        limit = 1;
        limit_err = angle - lostop;
        return true;
    }
    else if ( angle >= histop )
    {
        limit = 2;
        limit_err = angle - histop;
        return true;
    }
    else
    {
        limit = 0;
        return false;
    }
}


bool dxJointLimitMotor::addLimot( dxJoint *joint,
    dReal fps, dReal *J1, dReal *J2, dReal *pairRhsCfm, dReal *pairLoHi,
    const dVector3 ax1, int rotational )
{
    // if the joint is powered, or has joint limits, add in the extra row
    int powered = fmax > 0;
    if ( powered || limit )
    {
        dReal *J1Used = rotational ? J1 + GI2__JA_MIN : J1 + GI2__JL_MIN;
        dReal *J2Used = rotational ? J2 + GI2__JA_MIN : J2 + GI2__JL_MIN;

        dCopyVector3(J1Used, ax1);

        dxBody *b1 = joint->node[1].body;
        if ( b1 )
        {
            dCopyNegatedVector3(J2Used, ax1);
        }

        // linear limot torque decoupling step:
        //
        // if this is a linear limot (e.g. from a slider), we have to be careful
        // that the linear constraint forces (+/- ax1) applied to the two bodies
        // do not create a torque couple. in other words, the points that the
        // constraint force is applied at must lie along the same ax1 axis.
        // a torque couple will result in powered or limited slider-jointed free
        // bodies from gaining angular momentum.
        // the solution used here is to apply the constraint forces at the point
        // halfway between the body centers. there is no penalty (other than an
        // extra tiny bit of computation) in doing this adjustment. note that we
        // only need to do this if the constraint connects two bodies.

        dVector3 ltd = {0,0,0}; // Linear Torque Decoupling vector (a torque)
        if ( !rotational && b1 )
        {
            dxBody *b0 = joint->node[0].body;
            dVector3 c;
            c[0] = REAL( 0.5 ) * ( b1->posr.pos[0] - b0->posr.pos[0] );
            c[1] = REAL( 0.5 ) * ( b1->posr.pos[1] - b0->posr.pos[1] );
            c[2] = REAL( 0.5 ) * ( b1->posr.pos[2] - b0->posr.pos[2] );
            dCalcVectorCross3( ltd, c, ax1 );
            dCopyVector3(J1 + dxJoint::GI2__JA_MIN, ltd);
            dCopyVector3(J2 + dxJoint::GI2__JA_MIN, ltd);
        }

        // if we're limited low and high simultaneously, the joint motor is
        // ineffective
        if ( limit && ( lostop == histop ) ) powered = 0;

        if ( powered )
        {
            pairRhsCfm[GI2_CFM] = normal_cfm;
            if ( ! limit )
            {
                pairRhsCfm[GI2_RHS] = vel;
                pairLoHi[GI2_LO] = -fmax;
                pairLoHi[GI2_HI] = fmax;
            }
            else
            {
                // the joint is at a limit, AND is being powered. if the joint is
                // being powered into the limit then we apply the maximum motor force
                // in that direction, because the motor is working against the
                // immovable limit. if the joint is being powered away from the limit
                // then we have problems because actually we need *two* lcp
                // constraints to handle this case. so we fake it and apply some
                // fraction of the maximum force. the fraction to use can be set as
                // a fudge factor.

                dReal fm = fmax;
                if (( vel > 0 ) || ( vel == 0 && limit == 2 ) ) fm = -fm;

                // if we're powering away from the limit, apply the fudge factor
                if (( limit == 1 && vel > 0 ) || ( limit == 2 && vel < 0 ) ) fm *= fudge_factor;


                dReal fm_ax1_0 = fm*ax1[0], fm_ax1_1 = fm*ax1[1], fm_ax1_2 = fm*ax1[2];

                dxBody *b0 = joint->node[0].body;
                dxWorldProcessContext *world_process_context = b0->world->unsafeGetWorldProcessingContext();

                world_process_context->LockForAddLimotSerialization();

                if ( rotational )
                {
                    dxBody *b1 = joint->node[1].body;
                    if ( b1 != NULL )
                    {
                        dBodyAddTorque( b1, fm_ax1_0, fm_ax1_1, fm_ax1_2 );
                    }

                    dBodyAddTorque( b0, -fm_ax1_0, -fm_ax1_1, -fm_ax1_2 );
                }
                else
                {
                    dxBody *b1 = joint->node[1].body;
                    if ( b1 != NULL )
                    {
                        // linear limot torque decoupling step: refer to above discussion
                        dReal neg_fm_ltd_0 = -fm*ltd[0], neg_fm_ltd_1 = -fm*ltd[1], neg_fm_ltd_2 = -fm*ltd[2];
                        dBodyAddTorque( b0, neg_fm_ltd_0, neg_fm_ltd_1, neg_fm_ltd_2 );
                        dBodyAddTorque( b1, neg_fm_ltd_0, neg_fm_ltd_1, neg_fm_ltd_2 );

                        dBodyAddForce( b1, fm_ax1_0, fm_ax1_1, fm_ax1_2 );
                    }

                    dBodyAddForce( b0, -fm_ax1_0, -fm_ax1_1, -fm_ax1_2 );
                }

                world_process_context->UnlockForAddLimotSerialization();
            }
        }

        if ( limit )
        {
            dReal k = fps * stop_erp;
            pairRhsCfm[GI2_RHS] = -k * limit_err;
            pairRhsCfm[GI2_CFM] = stop_cfm;

            if ( lostop == histop )
            {
                // limited low and high simultaneously
                pairLoHi[GI2_LO] = -dInfinity;
                pairLoHi[GI2_HI] = dInfinity;
            }
            else
            {
                if ( limit == 1 )
                {
                    // low limit
                    pairLoHi[GI2_LO] = 0;
                    pairLoHi[GI2_HI] = dInfinity;
                }
                else
                {
                    // high limit
                    pairLoHi[GI2_LO] = -dInfinity;
                    pairLoHi[GI2_HI] = 0;
                }

                // deal with bounce
                if ( bounce > 0 )
                {
                    // calculate joint velocity
                    dReal vel;
                    if ( rotational )
                    {
                        vel = dCalcVectorDot3( joint->node[0].body->avel, ax1 );
                        if ( joint->node[1].body )
                            vel -= dCalcVectorDot3( joint->node[1].body->avel, ax1 );
                    }
                    else
                    {
                        vel = dCalcVectorDot3( joint->node[0].body->lvel, ax1 );
                        if ( joint->node[1].body )
                            vel -= dCalcVectorDot3( joint->node[1].body->lvel, ax1 );
                    }

                    // only apply bounce if the velocity is incoming, and if the
                    // resulting c[] exceeds what we already have.
                    if ( limit == 1 )
                    {
                        // low limit
                        if ( vel < 0 )
                        {
                            dReal newc = -bounce * vel;
                            if ( newc > pairRhsCfm[GI2_RHS] ) pairRhsCfm[GI2_RHS] = newc;
                        }
                    }
                    else
                    {
                        // high limit - all those computations are reversed
                        if ( vel > 0 )
                        {
                            dReal newc = -bounce * vel;
                            if ( newc < pairRhsCfm[GI2_RHS] ) pairRhsCfm[GI2_RHS] = newc;
                        }
                    }
                }
            }
        }
        return true;
    }
    return false;
}

/**
    This function generalizes the "linear limot torque decoupling"
    in addLimot to use anchor points provided by the caller.

    This makes it so that the appropriate torques are applied to
    a body when it's being linearly motored or limited using anchor points
    that aren't at the center of mass.

    pt1 and pt2 are centered in body coordinates but use global directions.
    I.e., they are conveniently found within joint code with:
      getAxis(joint,pt1,anchor1);
      getAxis2(joint,pt2,anchor2);
*/
bool dxJointLimitMotor::addTwoPointLimot( dxJoint *joint, dReal fps,
    dReal *J1, dReal *J2, dReal *pairRhsCfm, dReal *pairLoHi,
    const dVector3 ax1, const dVector3 pt1, const dVector3 pt2 )
{
    // if the joint is powered, or has joint limits, add in the extra row
    int powered = fmax > 0;
    if ( powered || limit )
    {
        // Set the linear portion
        dCopyVector3(J1 + GI2__JL_MIN, ax1);
        // Set the angular portion (to move the linear constraint
        // away from the center of mass).
        dCalcVectorCross3(J1 + GI2__JA_MIN, pt1, ax1);
        // Set the constraints for the second body
        if ( joint->node[1].body ) {
            dCopyNegatedVector3(J2 + GI2__JL_MIN, ax1);
            dCalcVectorCross3(J2 + GI2__JA_MIN, pt2, J2 + GI2__JL_MIN);
        }

        // if we're limited low and high simultaneously, the joint motor is
        // ineffective
        if ( limit && ( lostop == histop ) ) powered = 0;

        if ( powered )
        {
            pairRhsCfm[GI2_CFM] = normal_cfm;
            if ( ! limit )
            {
                pairRhsCfm[GI2_RHS] = vel;
                pairLoHi[GI2_LO] = -fmax;
                pairLoHi[GI2_HI] = fmax;
            }
            else
            {
                // the joint is at a limit, AND is being powered. if the joint is
                // being powered into the limit then we apply the maximum motor force
                // in that direction, because the motor is working against the
                // immovable limit. if the joint is being powered away from the limit
                // then we have problems because actually we need *two* lcp
                // constraints to handle this case. so we fake it and apply some
                // fraction of the maximum force. the fraction to use can be set as
                // a fudge factor.

                dReal fm = fmax;
                if (( vel > 0 ) || ( vel == 0 && limit == 2 ) ) fm = -fm;

                // if we're powering away from the limit, apply the fudge factor
                if (( limit == 1 && vel > 0 ) || ( limit == 2 && vel < 0 ) ) fm *= fudge_factor;


                const dReal* tAx1 = J1 + GI2__JA_MIN;
                dBodyAddForce( joint->node[0].body, -fm*ax1[dSA_X], -fm*ax1[dSA_Y], -fm*ax1[dSA_Z] );
                dBodyAddTorque( joint->node[0].body, -fm*tAx1[dSA_X], -fm*tAx1[dSA_Y], -fm*tAx1[dSA_Z] );

                if ( joint->node[1].body )
                {
                    const dReal* tAx2 = J2 + GI2__JA_MIN;
                    dBodyAddForce( joint->node[1].body, fm*ax1[dSA_X], fm*ax1[dSA_Y], fm*ax1[dSA_Z] );
                    dBodyAddTorque( joint->node[1].body, -fm*tAx2[dSA_X], -fm*tAx2[dSA_Y], -fm*tAx2[dSA_Z] );
                }

            }
        }

        if ( limit )
        {
            dReal k = fps * stop_erp;
            pairRhsCfm[GI2_RHS] = -k * limit_err;
            pairRhsCfm[GI2_CFM] = stop_cfm;

            if ( lostop == histop )
            {
                // limited low and high simultaneously
                pairLoHi[GI2_LO] = -dInfinity;
                pairLoHi[GI2_HI] = dInfinity;
            }
            else
            {
                if ( limit == 1 )
                {
                    // low limit
                    pairLoHi[GI2_LO] = 0;
                    pairLoHi[GI2_HI] = dInfinity;
                }
                else
                {
                    // high limit
                    pairLoHi[GI2_LO] = -dInfinity;
                    pairLoHi[GI2_HI] = 0;
                }

                // deal with bounce
                if ( bounce > 0 )
                {
                    // calculate relative velocity of the two anchor points
                    dReal vel =
  	                    dCalcVectorDot3( joint->node[0].body->lvel, J1 + GI2__JL_MIN ) +
  	                    dCalcVectorDot3( joint->node[0].body->avel, J1 + GI2__JA_MIN );
  	                if (joint->node[1].body) {
  	                    vel +=
  	                        dCalcVectorDot3( joint->node[1].body->lvel, J2 + GI2__JL_MIN ) +
  	                        dCalcVectorDot3( joint->node[1].body->avel, J2 + GI2__JA_MIN );
  	                }

                    // only apply bounce if the velocity is incoming, and if the
                    // resulting c[] exceeds what we already have.
                    if ( limit == 1 )
                    {
                        // low limit
                        if ( vel < 0 )
                        {
                            dReal newc = -bounce * vel;
                            if ( newc > pairRhsCfm[GI2_RHS] ) pairRhsCfm[GI2_RHS] = newc;
                        }
                    }
                    else
                    {
                        // high limit - all those computations are reversed
                        if ( vel > 0 )
                        {
                            dReal newc = -bounce * vel;
                            if ( newc < pairRhsCfm[GI2_RHS] ) pairRhsCfm[GI2_RHS] = newc;
                        }
                    }
                }
            }
        }
        return true;
    }
    return false;
}


// Local Variables:
// mode:c++
// c-basic-offset:4
// End:
