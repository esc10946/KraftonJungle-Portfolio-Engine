local State = {}

function State.ResetAttack(ctx)
    ctx.state.attackPlaying = false
    ctx.state.currentAttackIndex = 0
    ctx.state.canChainAttack = false
    ctx.state.attackInputQueued = false
end

function State.ResetGuard(ctx)
    ctx.state.defensePlaying = false
    ctx.state.defenseIdlePlaying = false
    ctx.state.successParryPlaying = false
    ctx.state.canCancelSuccessParry = false
    ctx.state.parryWindowOpen = false
end

function State.ResetHit(ctx)
    ctx.state.hitPlaying = false
    ctx.state.currentHitMontage = nil
    ctx.state.canCancelHit = false
end

function State.ResetCombat(ctx)
    State.ResetAttack(ctx)
    State.ResetGuard(ctx)
    State.ResetHit(ctx)
end

return State
