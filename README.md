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

General Improvements To Make:

Need to Add Drifting Force Timeline so that direction change is more dynamic

Need to add jumping, jump buffer and coyote time

Need to Add Rotation Code for Collider for visual effects

Might want a Timeline for speeding up and going slower the closer to max speed you are

Would like to seperate the parts that deal with physics and the character controller part into a pawn class that inherents from the physics pawn class


Would like to add some Stepping Logic so KCC can deal with steps and the like. This is a longer term goal.

Would also like to add some Wall Running Logic again as a longer term goal.

Changes that Could be made But I'm not sure about:

Could use Project On Plane instead of my current Project And Scale which keeps speed even when ascending a high slope

Could Instead of using 2 seperate Capsule Sweeps use 1 which would mean I would need to bake grounding logic inside the Collision Detection and Seperate The Vertical Component so that it doesn't contribute to the sliding magnitude.
If I did this there would also be the performance issue that every time I hit a climbable slope there would have to be an extra ProjectOnPlane to Remove Gravity Displacement

Could add sliding from gravity but I think this is worse as it means that you cant plainly stand anywhere on a slope

Issues:

KCC is bouncing off the ground when hitting a slope also seems to speed up when on slopes

Very Rarely the collider can get stuck inside another collider/surface

When on very high slopes slows down and veers in a lateral direction moving very slowly

Friction max velocity may need some fine tuning

Need to get Skeletal Mesh to actually show up and work
