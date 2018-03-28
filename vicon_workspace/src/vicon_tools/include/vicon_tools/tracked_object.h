#ifndef VICON_TOOLS_TRACKED_OBJECT_H
#define VICON_TOOLS_TRACKED_OBJECT_H

// Vicon tools
#include "vicon_tools/vector3D.h"         // Vector3D

class TrackedObject
{
    public:
        // Constructor
        TrackedObject();

        // Getter for predicted position delta_time seconds from last know position
        struct Vector3D predictPosition(double delta_time);

        // Updates position and velocity
        void updatePosition(struct Vector3D new_pos, double delta_time);

        // Holds whether object is initialized
        bool isInitialized_;

        // Position
        struct Vector3D pos_;
		
    private:

        // Velocity
        struct Vector3D vel_;
};

#endif // VICON_TOOLS_TRACKED_OBJECT_H