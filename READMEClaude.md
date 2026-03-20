# READMEClaude

## Purpose
This document is a fast handoff for any AI/code collaborator (including Claude/Codex) joining the project.

Project: `MyProject` (UE5 C++)
Repo: `https://github.com/ericginos/myproject-ue5-hero-soccer`
Primary gameplay module path: `Source/MyProject/source`

## Current Status (Implemented)
- Multiplayer-ready hero soccer foundation in C++ for 5v5.
- Core classes created and wired:
  - Custom `GameMode`, `GameState`, `PlayerState`, `PlayerController`
  - Character base classes and hero role classes
  - Team system (`TeamA`/`TeamB`), score, timer, match flow
- Replicated attribute system:
  - Health, Stamina, MovementSpeed, KickPower, PassAccuracy, AbilityCooldownReduction
- Soccer systems:
  - Replicated physics soccer ball actor
  - Kick / pass / charged shot / steal possession
  - Goal overlap detection and score handling
- Hero framework:
  - Base hero + lightweight custom ability component
  - Goalie, Defender, Midfielder, Forward classes and role abilities
- Multiplayer/UI systems:
  - Team selection + hero selection replication path
  - HUD data binding path to replicated gameplay state
- Security/performance passes completed:
  - Server-authority anti-cheat hardening across RPC paths
  - Replication optimization for 10-player match (relevancy, cull distance, conditional replication)

## Prompt History Used (Chronological)
1. Create scalable multiplayer architecture in UE5 C++ for a hero-based soccer game (5v5), with GameMode/GameState/PlayerState/PlayerController/Character base/teams/timer/score and proper replication macros.
2. Create modular replicated Attribute Component with: Health, Stamina, MovementSpeed, KickPower, PassAccuracy, AbilityCooldownReduction.
3. Create replicated SoccerBall actor with physics simulation, kick impulse, possession detection, goal overlap detection, movement replication, and authority checks.
4. Implement ball interaction system: kick, pass, charged shots, steal possession; server-authoritative, anti-cheat oriented, replicated physics, cooldown handling.
5. Create goal detection and scoring system with goal validation, GameState score updates, ball reset, player reset, multicast celebration, proper match flow.
6. Create `BaseHeroCharacter`: attributes + abilities, sprinting/stamina drain, ability activation via input, server RPC + multicast effects.
7. Create lightweight modular `AbilityComponent`: cooldowns, stamina costs, server validation, activation replication, montage triggering, example `DashBoost`.
8. Create `GoalieHero`: Dive Save, Goal Shield, Power Punt; tank-hybrid stats, fully replicated.
9. Create `DefenderHero`: Body Check, Intercept Dash, Defensive Aura; high durability, multiplayer safe.
10. Create `MidfielderHero`: Speed Boost Aura, Precision Pass (auto-aim assist), Stamina Regen Field; team-based support utility.
11. Create `ForwardHero`: Power Shot, Curve Shot, Breakaway Dash; highest kick power.
12. Implement Team Selection System: join Team A/B, select hero class, spawn at team spawn, replicated lobby flow.
13. Implement Match Flow System: warmup, countdown, active match, overtime if tied, end match screen (replicated via GameState).
14. Implement replicated Stamina & Sprint System.
15. Implement server-authoritative Crowd Control System: knockback, slow, stun with anti-stack handling.
16. Implement competitive Camera & Targeting: dynamic sprint FOV, soft-lock pass targeting, ball tracking priority.
17. Create multiplayer HUD showing health/stamina/ability cooldowns/team score/match timer bound to replicated values.
18. Create Hero Selection UI with role categories, abilities, locked heroes support, and server-side selection replication.
19. Anti-Cheat Authority Audit: remove/limit client trust and harden server validation paths.
20. Performance Optimization pass for 10-player replication load:
   - relevancy logic
   - `NetCullDistanceSquared` tuning
   - conditional replication tuning

## Notable Hardening/Optimization Notes
- Client-driven raw attribute mutation RPC was disabled; authoritative gameplay code updates attributes on server.
- Ability and ball-interaction server RPCs were validated more strictly.
- Team/hero selection payloads were sanitized server-side.
- Some `AlwaysRelevant` usage was reduced in favor of relevancy + cull distance settings.
- Owner-only / skip-owner replication was applied for cooldown/state fields where appropriate.

## Build/Run Notes
- Engine association currently: UE `5.7`
- Typical build command used:
  - `Engine/Build/BatchFiles/Build.bat MyProjectEditor Win64 Development -Project="<path>\MyProject.uproject" -WaitMutex -FromMsBuild`
- Multiplayer local test path:
  - Set world `GameMode Override` to soccer GameMode (or BP subclass)
  - PIE players = 2 then scale to 10
  - Test Listen Server and Dedicated Server modes

## Suggested Next Steps for New Agent
1. Add automated multiplayer smoke tests (spawn/team select/goal score/reset).
2. Add replication graph or further actor-channel tuning if scaling beyond 10 players.
3. Add ability data assets + balance tables (separate tuning from code).
4. Finalize UI/UX flow for lobby -> selection -> kickoff -> post-match.
5. Add server-side telemetry hooks (ability usage, possession time, goal events).

## Collaboration Ground Rules
- Keep gameplay logic server-authoritative.
- Preserve existing replication patterns unless performance profiling says otherwise.
- Prefer adding tests and instrumentation when touching networking code.
