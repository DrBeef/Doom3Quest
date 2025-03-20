#if !defined(vr_client_info_h)
#define vr_client_info_h

#ifdef __cplusplus
extern "C" {
#endif

typedef float vec2_t[2];
typedef float vec3_t[3];
typedef float vec4_t[4];

typedef struct {
    bool screen;
    float fov;
    bool oneHandOnly;
    bool right_handed;
    bool player_moving;
    bool visible_hud;
    bool weapon_stabilised;
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

    //Lubos BEGIN
    int weaponZooming;
    float snapTurn;
    bool consoleShown;
    bool disableFootStep;
    bool inMenu;
    bool vehicleMode;
    bool weaponZoom;
    float vehicleYaw;
    char* levelname;
    bool lockedCamera;
    bool credits;
    bool weaponGun;
    bool weaponModifier;
    bool weaponTwoHand;
    vec3_t weaponOffset;
    vec3_t hmdorientation_diff;
    vec3_t hmdorientation_offset;
    vec3_t hmdorientation_prev;
    vec2_t uiScale;
    vec2_t uiOffset;
    vec3_t playerPosition;
    vec3_t vehicleOffset;
    //Lubos END

    //FP - Temp Variables for other stuff
    vec3_t hmdorientation_temp;
    vec3_t weaponangles_temp;
    vec3_t weaponangles_last_temp; // Don't use this, it is just for calculating delta!
    vec3_t weaponangles_delta_temp;

    vec3_t throw_origin;
    vec3_t throw_trajectory;
    float throw_power;

    bool velocitytriggered; // Weapon attack triggered by velocity (knife)
    bool velocitytriggeredoffhand; // Weapon attack triggered by velocity (puncher)
    bool velocitytriggeredoffhandstate; // Weapon attack triggered by velocity (puncher)

    vec3_t offhandangles_temp;
    vec3_t offhandoffset_temp;
} vrClientInfo;

extern vrClientInfo *pVRClientInfo;

#ifdef __cplusplus
}
#endif

#endif //vr_client_info_h