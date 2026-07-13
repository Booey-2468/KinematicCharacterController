# KinematicCharacterController

What The Project is Like Now?:

Currently I have got a working KCC using The Collide and Slide Algorithm by K. Fauerby
Used this Tutorial https://www.youtube.com/watch?v=YR6Q7dUz2uk as my collision detection method.

Uses My own custom Force Accumulation System calculating physics forces such as simple gravity, drag and friction

This Force Accumulation System also allows for transform velocity input allowing for an instant switch from Force to Transform-based Movement.

I also have many different Time Integration methods being: Euler, Verlet, Velocity Verlet and RK4. I am currently using Velocity Verlet.

I also implemented my own Vector Mathematics as needed such as dot and cross product, Vector Projection on to normals and planes, among others

All my Systems use m, m/s etc. so I convert any incoming information from UE5 such as location or hit distance from Capsule Sweeps and vice  versa.

Player Movement is based on Camera rotation projected onto a plane based on gravity.

I also have Curves for quick speeding up and turning as well as a cornering force for better turning

I now have a fairly basic jumping system allowing multiple jumps with Coyote Time and Jump Buffer as well as variable jump height which uses impulses and can be customised based on minimum jump time and the downward impulses minimum

I now have added Stepping Logic so when meeting a wall smaller than step height it should teleport on top of it if there is available space

Now on all slopes 50 degrees or under there is perfect movement

General Improvements To Make:

Would like to seperate the parts that deal with physics and the character controller part into a pawn class that inherents from the physics pawn class

Get No periodic slowing on medium slopes and No Slowness and veering on high slopes.

Also preferably no periodic speed boost when sliding along walls.

Would also like to add some Wall Running Logic again as a longer term goal.

Changes that Could be made But I'm not sure about:

Could Instead of using 2 seperate Capsule Sweeps use 1 which would mean I would need to bake grounding logic inside the Collision Detection and Seperate The Vertical Component so that it doesn't contribute to the sliding magnitude.
If I did this there would also be the performance issue that every time I hit a climbable slope there would have to be an extra ProjectOnPlane to Remove Gravity Displacement

Issues:

Would like to get stepping working a bit better as sometimes you collide instead of step up

When on a slope of 60 degrees goes up it slowly but this may be due to there not being a big enough grounding check
When on high slopes e.g. 70+ slows down and veers in a lateral direction moving very slowly or on lower slopes experiences a bit of bouncing

Ok so The collision detection does not work when on the underside of a plane or ejects you into space

Friction max velocity may need some fine tuning
KCC bouncing is now only really present on higher slopes and those already have issues also changing to use project on plane does not help


Need to get Skeletal Mesh to actually show up and work
