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

I now have added Stepping Logic so when meeting a wall smaller than step height it should teleport on top of it to a certain extent

General Improvements To Make:

Would like to seperate the parts that deal with physics and the character controller part into a pawn class that inherents from the physics pawn class


Would also like to add some Wall Running Logic again as a longer term goal.

Changes that Could be made But I'm not sure about:

Could use Project On Plane instead of my current Project And Scale which keeps speed even when ascending a high slope

Could Instead of using 2 seperate Capsule Sweeps use 1 which would mean I would need to bake grounding logic inside the Collision Detection and Seperate The Vertical Component so that it doesn't contribute to the sliding magnitude.
If I did this there would also be the performance issue that every time I hit a climbable slope there would have to be an extra ProjectOnPlane to Remove Gravity Displacement

Could add sliding from gravity but I think this is worse as it means that you cant plainly stand anywhere on a slope

Issues:
Need to get Skeletal Mesh to actually show up

When on very high slopes slows down and veers in a lateral direction moving very slowly

Ok so The collision detection does not work when on the underside of a plane or ejects you into space

Also I think my code now handles creases pretty well as it uses a depenetration measure to move back just in case it does get stuck which it can and it treats creases as walls by projecting the floor normal on to gravity
Can have some issues when moving diagonally though so not a full amazing fix but I think this is the sweeps fault not mine though I could be wrong

Friction max velocity may need some fine tuning
KCC bouncing has been severly reduced but might still be there a bit


Need to get Skeletal Mesh to actually show up and work
