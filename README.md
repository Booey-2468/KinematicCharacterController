# KinematicCharacterController

What The Project is Like Now?:

Currently I have got a working KCC using The Collide and Slide Algorithm by K. Fauerby
Used this Tutorial https://www.youtube.com/watch?v=YR6Q7dUz2uk as my collision detection method.

Uses My own custom Force Accumulation System calculating physics forces such as simple gravity, drag and friction

This Force Accumulation System also allows for transform velocity input allowing for an instant switch from Force to Transform-based Movement.

I also have many different Time Integration methods being: Euler, Verlet, Velocity Verlet and RK4. I am currently using Velocity Verlet.

I also implemented my own Vector Mathematics as needed such as dot and cross product, Vector Projection on to normals and planes, among others

All my Systems use m, m/s etc. so I convert any incoming information from UE5 such as location or hit distance from Capsule Sweeps and vice  versa.

Player Movement is based on Camera rotation projected onto a plane based on gravity. Also uses extra drag when grounded to prevent sliding.

I also have Curves for quick speeding up and turning as well as a cornering force for better turning

I now have a fairly basic jumping system allowing multiple jumps with Coyote Time and Jump Buffer as well as variable jump height which uses impulses and can be customised based on minimum jump time and the downward impulses minimum

I now have added Stepping Logic so when meeting a wall smaller than step height it should teleport on top of it if there is available space and keeps remaining velocity for smooth movement.

Now on all slopes 60 degrees or under there is perfect movement this is done by adding an extra skin width only for the gravity displacement

When colliding with a physics object via the collide and slide algorithm applies a collision impulse at the impact location. Also if something has hit the actual collider there is penetration resolution along with an impulse applied to both the physics object if it is one and the player. 

Added Properties to the Details page.

Separated the physics and player movement parts of the Kinematic Character Controller so that the physics part can be used for many things and be customised by anyone

General Improvements To Make:

Would also like to add some Wall Running Logic again as a longer term goal.

Changes that Could be made But I'm not sure about:

Could Instead of using 2 seperate Capsule Sweeps use 1 which would mean I would need to bake grounding logic inside the Collision Detection and Seperate The Vertical Component so that it doesn't contribute to the sliding magnitude.
If I did this there would also be the performance issue that every time I hit a climbable slope there would have to be an extra ProjectOnPlane to Remove Gravity Displacement

With the collisions with 

Issues:

Got a big issue with call stack where it calls a breakpoint think it has something to do with pointers or references mainly happened with acceleration but also with current bounces. This doesn't crash the editor though

Preferably no periodic speed boost when sliding along walls.

Would like to get stepping working a bit better as sometimes you collide instead of step up.

Can experience issues of bouncing a bit instead of gliding up the surface this is likely due to the big issue with little collision corrections causing larger upwards velocity which also slowed me down on slopes as when in the air speed is greatly reduced.

Sometimes when moving camera and character movement can experience a bit of jitter though that could maybe be due to frame rate though I am not sure.

Friction is probably good though you can slightly see if you focus the player moving.



Need to get Skeletal Mesh to actually show up and work
