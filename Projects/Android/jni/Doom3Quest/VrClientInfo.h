#if !defined(vr_client_info_h)
#define vr_client_info_h

#define NUM_WEAPON_SAMPLES      7
#define OLDER_READING		    (NUM_WEAPON_SAMPLES-1)
#define NEWER_READING		    (NUM_WEAPON_SAMPLES-5)

#ifdef __cplusplus
extern "C" {
#endif

typedef float vec3_t[3];
typedef float vec4_t[4];

typedef struct {
    bool screen;
    float fov;
    bool weapon_stabilised;
    bool oneHandOnly;
    bool right_handed;
    bool player_moving;
    bool visible_hud;
    bool dualwield;
    int weaponid;
    int lastweaponid;

    //FP - Carry original values
    vec4_t hmdorientation_quat;
    vec3_t hmdtranslation;
    vec3_t lhandposition;
    vec3_t rhandposition;
    vec4_t lhand_orientation_quat;
    vec4_t rhand_orientation_quat;


    vec3_t hmdposition;
    vec3_t hmdposition_last; // Don't use this, it is just for calculating delta!
    vec3_t hmdposition_delta;

    //FP - Temp Variables for other stuff
    vec3_t hmdorientation_temp;
    vec3_t weaponangles_temp;
    vec3_t weaponangles_last_temp; // Don't use this, it is just for calculating delta!
    vec3_t weaponangles_delta_temp;

    /*vec3_t hmdorientation;
    vec3_t hmdorientation_last; // Don't use this, it is just for calculating delta!
    vec3_t hmdorientation_delta;*/

    /*vec3_t weaponangles_unadjusted;
    vec3_t weaponangles;
    vec3_t weaponangles_last; // Don't use this, it is just for calculating delta!
    vec3_t weaponangles_delta;*/

    //vec3_t flashlightHolsterOffset; // Where the flashlight can be picked up from

    /*vec3_t current_weaponoffset;
    float current_weaponoffset_timestamp;
    vec3_t weaponoffset_history[NUM_WEAPON_SAMPLES];
    float weaponoffset_history_timestamp[NUM_WEAPON_SAMPLES];*/

    vec3_t throw_origin;
    vec3_t throw_trajectory;
    float throw_power;

    bool velocitytriggered; // Weapon attack triggered by velocity (knife)
    bool velocitytriggeredoffhand; // Weapon attack triggered by velocity (puncher)
    bool velocitytriggeredoffhandstate; // Weapon attack triggered by velocity (puncher)

    vec3_t offhandangles_temp;
    vec3_t offhandoffset_temp;

    //
    // Teleport Stuff
    //
    /*bool teleportenabled;
    bool teleportseek; // player looking to teleport
    bool teleportready; // player pointing to a valid teleport location
    vec3_t teleportdest; // teleport destination
    bool teleportexecute; // execute the teleport*/



    //////////////////////////////////////
    //    Test stuff for weapon alignment
    //////////////////////////////////////

    /*char test_name[256];
    float test_scale;
    vec3_t test_angles;
    vec3_t test_offset;*/

} vrClientInfo;

extern vrClientInfo *pVRClientInfo;

#ifdef __cplusplus
}
#endif

#endif //vr_client_info_h