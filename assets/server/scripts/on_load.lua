function reset_game()
    sprf.tracelog(sprf.log_info,"resetting game")
    ball_start_pos,ball_start_rot = sprf.get_named_position("ball_start",0)
    sprf.set_ball_velocity(0,0,0)
    sprf.set_ball_position(ball_start_pos.x,ball_start_pos.y,ball_start_pos.z)
end

sprf.tracelog(sprf.log_info,"server loaded")

reset_game()