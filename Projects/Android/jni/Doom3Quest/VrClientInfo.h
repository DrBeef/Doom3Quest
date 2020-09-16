#if !defined(vr_client_info_h)
#define vr_client_info_h

#define NUM_WEAPON_SAMPLES      10
#define WEAPON_RECOIL           15.0f;

#ifdef __cplusplus
extern "C" {
#endif

typedef float vec3_t[3];

typedef struct {
    bool screen;
    float fov;
    bool weapon_stabilised;
    bool right_handed;
    bool player_moving;
    bool visible_hud;
    bool dualwield;
    int weaponid;
    int lastweaponid;
    int backpackitemactive; //0 - nothing, 1 - grenades, 2 - knife, 3 - Binoculars
    bool mountedgun;

    vec3_t hmdposition;
    vec3_t hmdposition_last; // Don't use this, it is just for calculating delta!
    vec3_t hmdposition_delta;

    vec3_t hmdorientation;
    vec3_t hmdorientation_last; // Don't use this, it is just for calculating delta!
    vec3_t hmdorientation_delta;

    vec3_t weaponangles_unadjusted;
    vec3_t weaponangles;
    vec3_t weaponangles_last; // Don't use this, it is just for calculating delta!
    vec3_t weaponangles_delta;

    vec3_t current_weaponoffset;
    vec3_t calculated_weaponoffset;
    float current_weaponoffset_timestamp;
    vec3_t weaponoffset_history[NUM_WEAPON_SAMPLES];
    float weaponoffset_history_timestamp[NUM_WEAPON_SAMPLES];

    bool pistol;                // True if the weapon is a pistol

    bool velocitytriggered; // Weapon attack triggered by velocity (knife)

    vec3_t offhandangles;
    vec3_t offhandoffset;

    //
    // Teleport Stuff
    //
    bool teleportenabled;
    bool teleportseek; // player looking to teleport
    bool teleportready; // player pointing to a valid teleport location
    vec3_t teleportdest; // teleport destination
    bool teleportexecute; // execute the teleport



    //////////////////////////////////////
    //    Test stuff for weapon alignment
    //////////////////////////////////////

    char test_name[256];
    float test_scale;
    vec3_t test_angles;
    vec3_t test_offset;

} vrClientInfo;

extern vrClientInfo *pVRClientInfo;

#ifdef __cplusplus
}
#endif

#endif //vr_client_info_h