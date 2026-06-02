# Cardputer Zero Board Facts

Sources:

- `docs/LINUX_ADAPTATION_GUIDE.md`
- `docs/targets/linux_targets.md`
- `platform/linux/common/src/core/display_profile.h`

This record describes current repo evidence for the Linux Cardputer Zero route.

## Identity

- board id: `cardputerzero`
- current build entry evidence: `builds/linux_cmake`
- current app shell evidence: `apps/linux_cardputer_zero`
- historical implementation evidence: `removed root linux_rpi`
- current shared Linux code: `platform/linux/common`

## Confirmed Facts

- logical display size used by current common Linux shell: 320 x 170
- keyboard input is required but real device key mapping still needs sampling
- display/input ownership for the real Pi OS path is not yet closed
- current dedicated app shell baseline is build-owned by
  `apps/linux_cardputer_zero`

## Pending Hardware Evidence

- framebuffer/display handoff on real Cardputer Zero hardware
- evdev keyboard mapping sampled from the real device
- launch/package route for the portable Linux device

The Linux simulator remains a separate target under `apps/linux_sim_shell`.
