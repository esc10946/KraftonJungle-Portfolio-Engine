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
    ctx.state.counterWindowOpen = false
end

function State.ResetCounter(ctx)
    ctx.state.counterPlaying = false
    ctx.state.canCancelCounter = false
    ctx.state.counterMove = nil
end

function State.ResetHit(ctx)
    ctx.state.hitPlaying = false
    ctx.state.currentHitMontage = nil
    ctx.state.canCancelHit = false
end

function State.ResetCombat(ctx)
    State.ResetAttack(ctx)
    State.ResetGuard(ctx)
    State.ResetCounter(ctx)
    State.ResetHit(ctx)
end

return State
