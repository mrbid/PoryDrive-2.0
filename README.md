* **Play Online:** https://mrbid.github.io/porydrive
* **Flathub:** https://flathub.org/apps/com.voxdsp.PoryDrive
* **Windows:** [PoryDrive_win.zip](https://github.com/mrbid/PoryDrive-2.0/releases/download/windows/PoryDrive_win.zip)
* How 3D assets are converted from ASCII PLY to C Header files: [assets/ptf.c](assets/ptf.c)

## input
*  **ESCAPE** = Unlock Mouse
*  **N** = New Game and Car Color
*  **W,A,S,D** = Drive Car
*  **Space** = Brake
*  **L-Shift** = Boost
*  **RIGHT CLICK/MOUSE4** = Zoom Snap Close/Ariel
*  **Mouse Scroll** = Zoom in/out
*  **C** = Random Car Colors
*  **R** = Accelerate/step DNA color peel
*  **F** = FPS to console
*  **P** = Player stats to console
*  **O** = Toggle auto drive

## config
It is possible to tweak the car physics by creating a `config.txt` file in the exec/working directory of the game, here is an example of such config file with the default car physics variables.
```
maxspeed 0.0265
acceleration 0.0028
inertia 0.0022
drag 0.00038
steeringspeed 0.04
steerinertia 120
minsteer 0.3
maxsteer 0.36
steering_deadzone 0.033
steeringtransfer 0.023
steeringtransferinertia 280
suspension_pitch 3
suspension_pitch_limit 0.06
suspension_roll 30
suspension_roll_limit 0.3
sticky_collisions 0

ad_min_dstep 0.01
ad_max_dstep 0.06
ad_min_speedswitch 2
ad_maxspeed_reductor 0.5
```
#### car physics variables
- `maxspeed` - top travel speed of car.
- `acceleration` - increase of speed with respect to time.
- `inertia` - minimum speed before car will move from a stationary state.
- `drag` - loss in speed with respect to time.
- `steeringspeed` - how fast the wheels turn.
- `steerinertia` - how much of the max steering angle is lost as the car increases in speed _(crude steering loss)_.
- `minsteer` - minimum steering angle as scalar _(1 = 180 degree)_ attainable after steering loss caused by `steeringintertia`.
- `maxsteer` - maximum steering angle as scalar _(1 = 180 degree)_ attainable at minimal speeds.
- `steering_deadzone` - minimum angle of steering considered the deadzone or cutoff, within this angle the steering angle will always be forced to zero.
- `steeringtransfer` - how much the wheel rotation angle translates into rotation of the body the wheels are connected to _(the car)_.
- `steeringtransferinertia` - how much the `steeringtransfer` reduces as the car speed increases, this is related to `steerinertia` to give the crude effect of traction loss of the front tires as speed increases and the inability to force the wheels into a wider angle at higher speeds.
- `suspension_pitch` - suspension pitch increment scalar
- `suspension_pitch_limit` - max & min pitch limit
- `suspension_roll` - suspension roll increment scalar
- `suspension_roll_limit` - max & min roll limit
- `sticky_collisions` - 0 = bouncy collisions, 1 = sticky collisions

#### auto drive variables
- `ad_min_dstep` - minimum delta-distance from the porygon that can trigger a change in steering direction. The delta-distance is the amount of change in the distance since the last update.
- `ad_max_dstep` - maximum delta-distance from the porygon, once this is set any distance above this limit will trigger a change in steering direction.
- `ad_min_speedswitch` - minimum distance from the porygon before the speed of the car begins to linearly reduce as it approaches the porygon.
- `ad_maxspeed_reductor` - the rate at which the speed reduces as the car approaches the porygon with respect to `ad_min_speedswitch`.

## attrib
* BMW E34 Model is made by [Krzysztof Stolorz](https://sketchfab.com/KrStolorz) (KrStolorz)
* I am using icons from [Forrest Walter](http://www.forrestwalter.com).
