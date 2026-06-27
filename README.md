# KinematicCharacterController

General Improvements To Make:
Need to Add Drifting Force Timeline so that direction change is more dynamic

Need to add jumping, jump buffer and coyote time

Might want a Timeline for speeding up and going slower the closer to max speed you are

Would like to seperate the parts that deal with physics and the character controller part into a pawn class that inherents from the physics pawn class


Issues:
KCC is bouncing off the ground when hitting a slope also seems to speed up when on slopes

When on very high slopes slows down and veers in a lateral direction moving very slowly

Friction max velocity may need some fine tuning
