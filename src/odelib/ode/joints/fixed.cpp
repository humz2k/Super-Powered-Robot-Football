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


#include <ode/odeconfig.h>
#include "../config.h"
#include "fixed.h"
#include "joint_internal.h"



//****************************************************************************
// fixed joint

dxJointFixed::dxJointFixed ( dxWorld *w ) :
    dxJoint ( w )
{
    dSetZero ( offset, 4 );
    dSetZero ( qrel, 4 );
    erp = world->global_erp;
    cfm = world->global_cfm;
}


void
dxJointFixed::getSureMaxInfo( SureMaxInfo* info )
{
    info->max_m = 6;
}


void
dxJointFixed::getInfo1 ( dxJoint::Info1 *info )
{
    info->m = 6;
    info->nub = 6;
}


void
dxJointFixed::getInfo2 ( dReal worldFPS, dReal worldERP,
    int rowskip, dReal *J1, dReal *J2,
    int pairskip, dReal *pairRhsCfm, dReal *pairLoHi,
    int *findex )
{
    // Three rows for orientation
    setFixedOrientation ( this, worldFPS, worldERP,
        rowskip, J1 + dSA__MAX * rowskip, J2 + dSA__MAX * rowskip,
        pairskip, pairRhsCfm + dSA__MAX * pairskip, qrel );

    // Three rows for position.
    // set Jacobian
    J1[GI2_JLX] = 1;
    J1[rowskip + GI2_JLY] = 1;
    J1[2 * rowskip + GI2_JLZ] = 1;

    dReal k = worldFPS * this->erp;
    dxBody *b0 = node[0].body, *b1 = node[1].body;

    dVector3 ofs;
    dMultiply0_331 ( ofs, b0->posr.R, offset );

    if ( b1 ) {
        dSetCrossMatrixPlus( J1 + GI2__JA_MIN, ofs, rowskip );

        J2[GI2_JLX] = -1;
        J2[rowskip + GI2_JLY] = -1;
        J2[2 * rowskip + GI2_JLZ] = -1;
    }

    // set right hand side for the first three rows (linear)
    if ( b1 ) {
        for ( int j = 0, currPairSkip = 0; j < 3; currPairSkip += pairskip, ++j ) {
            pairRhsCfm[currPairSkip + GI2_RHS] = k * ( b1->posr.pos[j] - b0->posr.pos[j] + ofs[j] );
        }
    } else {
        for ( int j = 0, currPairSkip = 0; j < 3; currPairSkip += pairskip, ++j ) {
            pairRhsCfm[currPairSkip + GI2_RHS] = k * ( offset[j] - b0->posr.pos[j] );
        }
    }

    dReal cfm = this->cfm;
    pairRhsCfm[GI2_CFM] = cfm;
    pairRhsCfm[pairskip + GI2_CFM] = cfm;
    pairRhsCfm[2 * pairskip + GI2_CFM] = cfm;
}


void dJointSetFixed ( dJointID j )
{
    dxJointFixed* joint = ( dxJointFixed* ) j;
    dUASSERT ( joint, "bad joint argument" );
    checktype ( joint, Fixed );
    int i;

    // This code is taken from dJointSetSliderAxis(), we should really put the
    // common code in its own function.
    // compute the offset between the bodies
    if ( joint->node[0].body )
    {
        if ( joint->node[1].body )
        {
            dReal ofs[4];
            for ( i = 0; i < 4; i++ )
                ofs[i] = joint->node[0].body->posr.pos[i] - joint->node[1].body->posr.pos[i];
            dMultiply1_331 ( joint->offset, joint->node[0].body->posr.R, ofs );
        }
        else
        {
            joint->offset[0] = joint->node[0].body->posr.pos[0];
            joint->offset[1] = joint->node[0].body->posr.pos[1];
            joint->offset[2] = joint->node[0].body->posr.pos[2];
        }
    }

    joint->computeInitialRelativeRotation();
}

void dxJointFixed::set ( int num, dReal value )
{
    switch ( num )
    {
    case dParamCFM:
        cfm = value;
        break;
    case dParamERP:
        erp = value;
        break;
    }
}


dReal dxJointFixed::get ( int num )
{
    switch ( num )
    {
    case dParamCFM:
        return cfm;
    case dParamERP:
        return erp;
    default:
        return 0;
    }
}


void dJointSetFixedParam ( dJointID j, int parameter, dReal value )
{
    dxJointFixed* joint = ( dxJointFixed* ) j;
    dUASSERT ( joint, "bad joint argument" );
    checktype ( joint, Fixed );
    joint->set ( parameter, value );
}


dReal dJointGetFixedParam ( dJointID j, int parameter )
{
    dxJointFixed* joint = ( dxJointFixed* ) j;
    dUASSERT ( joint, "bad joint argument" );
    checktype ( joint, Fixed );
    return joint->get ( parameter );
}


dJointType
dxJointFixed::type() const
{
    return dJointTypeFixed;
}


sizeint
dxJointFixed::size() const
{
    return sizeof ( *this );
}

void
dxJointFixed::computeInitialRelativeRotation()
{
    if (node[0].body )
    {
        if (node[1].body )
        {
            dQMultiply1 (qrel, node[0].body->q, node[1].body->q );
        }
        else
        {
            // set qrel to the transpose of the first body q
            qrel[0] =  node[0].body->q[0];
            qrel[1] = -node[0].body->q[1];
            qrel[2] = -node[0].body->q[2];
            qrel[3] = -node[0].body->q[3];
        }
    }
}

