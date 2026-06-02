# Cardputer Zero Linux App Shell

This app shell is the Trail Mate product route for the Cardputer Zero portable
Linux device. It is not the Linux simulator.

The first slice intentionally keeps the runtime entry small: the shell validates
the `cardputerzero` target profile, consumes the `boards/cardputerzero` facts,
selects the `cardputer_compact` UX pack, and builds through
`builds/linux_cmake`. That gives the device a real product boundary before the
framebuffer, evdev, launch, and package owners are filled in.

Known hardware facts in this repo:

- display: 320 x 170 logical pixels
- input: built-in keyboard
- touch, pointer, and trackball: absent in current board facts

The simulator remains a separate development shell under `apps/linux_sim_shell`.
