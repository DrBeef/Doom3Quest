package com.drbeef.doom3quest.bhaptics;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.support.v4.app.ActivityCompat;
import android.util.Log;

import com.bhaptics.bhapticsmanger.BhapticsManager;
import com.bhaptics.bhapticsmanger.BhapticsManagerCallback;
import com.bhaptics.bhapticsmanger.BhapticsModule;
import com.bhaptics.bhapticsmanger.DefaultHapticStreamer;
import com.bhaptics.bhapticsmanger.HapticStreamer;
import com.bhaptics.bhapticsmanger.HapticPlayer;
import com.bhaptics.commons.PermissionUtils;
import com.bhaptics.commons.model.BhapticsDevice;
import com.bhaptics.commons.model.DotPoint;
import com.bhaptics.commons.model.PositionType;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.UUID;
import java.util.Vector;



public class bHaptics {
    
    public static class Haptic
    {
        Haptic(PositionType type, String key, String altKey, String group, float intensity, float duration) {
            this.type = type;
            this.key = key;
            this.altKey = altKey;
            this.group = group;
            this.intensity = intensity;
            this.duration = duration;
        }

        public final String key;
        public final String altKey;
        public final String group;
        public final float intensity;
        public final float duration;
        public final PositionType type;
    };
    
    private static final String TAG = "Doom3Quest.bHaptics";

    private static Random rand = new Random();

    private static boolean hasPairedDevice = false;

    private static boolean enabled = false;
    private static boolean requestingPermission = false;
    private static boolean initialised = false;

    private static HapticPlayer player;
    private static HapticStreamer hapticStreamer;

    private static Context context;

    private static Map<String, Vector<Haptic>> eventToEffectKeyMap = new HashMap<>();

    private static  Map<String, Haptic> repeatingHaptics = new HashMap<>();

    public static void initialise()
    {
        if (initialised)
        {
            //Already initialised, but might need to rescan
            scanIfNeeded();
            return;
        }

        BhapticsModule.initialize(context);

        scanIfNeeded();

        player = BhapticsModule.getHapticPlayer();

        /*
            DAMAGE
        */
        registerFromAsset(context, "bHaptics/Damage/Body_Heartbeat.tact", PositionType.Vest, "heartbeat", "health", 1.0f, 1.0f);

        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Melee1.tact", "melee_left", "damage");
        registerFromAsset(context, "bHaptics/Damage/Head/DMG_Melee1.tact", PositionType.Head, "melee_left", "damage"); // always play melee on the head too

        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Melee2.tact", "melee_right", "damage");
        registerFromAsset(context, "bHaptics/Damage/Head/DMG_Melee2.tact", PositionType.Head, "melee_right", "damage"); // always play melee on the head too

        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Fireball.tact", "fireball", "damage");
        registerFromAsset(context, "bHaptics/Damage/Head/DMG_Explosion.tact", PositionType.Head, "fireball", "damage"); // always play fireball on the head too

        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Bullet.tact", "bullet", "damage");
        registerFromAsset(context, "bHaptics/Damage/Head/DMG_HeadShot.tact", PositionType.Head, "bullet", "damage");

        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Shotgun.tact", "shotgun", "damage");
        registerFromAsset(context, "bHaptics/Damage/Head/DMG_Explosion.tact", PositionType.Head, "shotgun", "damage");

        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Fire.tact", "fire", "damage");
        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Fire.tact", "noair", "damage");
        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Falling.tact", "fall", "damage");
        registerFromAsset(context, "bHaptics/Damage/Body_Shield_Break.tact", "shield_break", "damage");

         /*
            INTERACTIONS
         */
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Shield_Get.tact", "pickup_shield", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Pickup_L.tact", PositionType.ForearmL, "pickup_shield", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Pickup_R.tact", PositionType.ForearmR, "pickup_shield", "pickup");

        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Weapon_Get.tact", "pickup_weapon", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/ItemPickup_Mirror.tact", PositionType.ForearmL, "pickup_weapon", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/ItemPickup.tact", PositionType.ForearmR, "pickup_weapon", "pickup");

        //registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Ammo_Get.tact", "pickup_ammo", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Ammo_L.tact", PositionType.ForearmL, "pickup_ammo", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Ammo_R.tact", PositionType.ForearmR, "pickup_ammo", "pickup");

        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Healstation.tact", "healstation", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Healthstation_L.tact", PositionType.ForearmL, "healstation", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Healthstation_R.tact", PositionType.ForearmR, "healstation", "pickup");

        registerFromAsset(context, "bHaptics/Interaction/Vest/DoorSlide.tact", "doorslide", "door");
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Scan.tact", PositionType.Vest, "scan", "environment", 1.0f, 1.15f);
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Scan.tact", PositionType.Vest, "decontaminate", "environment", 0.5f, 0.75f);
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Chamber_Up.tact", "liftup", "environment");
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Chamber_Down.tact", "liftdown", "environment");
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Machine.tact", "machine", "environment");
        registerFromAsset(context, "bHaptics/Interaction/Vest/Spark.tact", "spark", "environment");
        registerFromAsset(context, "bHaptics/Interaction/Head/Spark.tact", PositionType.Head, "spark", "environment");

        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_PDA_Open.tact", "pda_open", "pda");
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_PDA_Open.tact", "pda_close", "pda");
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_PDA_Alarm.tact", "pda_alarm", "pda");
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_PDA_Touch.tact", "pda_touch", "pda");
        registerFromAsset(context, "bHaptics/Interaction/Arms/PDA_Click_Mirror.tact", PositionType.ForearmL, "pda_touch", "pda");
        registerFromAsset(context, "bHaptics/Interaction/Arms/PDA_Click.tact", PositionType.ForearmR, "pda_touch", "pda");


        registerFromAsset(context, "bHaptics/Interaction/Vest/PlayerJump.tact", "jump_start", "player");
        registerFromAsset(context, "bHaptics/Interaction/Vest/PlayerLanding.tact", "jump_landing", "player");


        /*
            WEAPONS
         */

        registerFromAsset(context, "bHaptics/Weapon/Vest/WeaponSwap.tact", PositionType.Right, "weapon_switch", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Swap_R.tact", PositionType.ForearmR, "weapon_switch", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Vest/WeaponSwap_Mirror.tact", PositionType.Left, "weapon_switch", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Swap_L.tact", PositionType.ForearmL, "weapon_switch", "weapon");

        //Reload Start
        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Reload.tact", "weapon_reload", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Reload_L.tact", PositionType.ForearmL, "weapon_reload", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Reload_R.tact", PositionType.ForearmR, "weapon_reload", "weapon");

        //Reload Finish
        registerFromAsset(context, "bHaptics/Weapon/Vest/ReloadFinish.tact", PositionType.Right, "weapon_reload_finish", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Arms/ReloadFinish.tact", PositionType.ForearmR, "weapon_reload_finish", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Vest/ReloadFinish_Mirror.tact", PositionType.Left, "weapon_reload_finish", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Arms/ReloadFinish_Mirror.tact", PositionType.ForearmL, "weapon_reload_finish", "weapon");

        //Chainsaw Idle
        registerFromAsset(context, "bHaptics/Weapon/Vest/Chainsaw_LV1.tact", PositionType.Right, "chainsaw_idle", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Chainsaw_LV1.tact", PositionType.ForearmR, "chainsaw_idle", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Chainsaw_LV1_Mirror.tact", PositionType.Left, "chainsaw_idle", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Chainsaw_LV1_Mirror.tact", PositionType.ForearmL, "chainsaw_idle", "weapon");

        //Chainsaw Fire
        registerFromAsset(context, "bHaptics/Weapon/Vest/Chainsaw_LV2.tact", PositionType.Right, "chainsaw_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Chainsaw_LV2.tact", PositionType.ForearmR, "chainsaw_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Chainsaw_LV2_Mirror.tact", PositionType.Left, "chainsaw_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Chainsaw_LV2_Mirror.tact", PositionType.ForearmL, "chainsaw_fire", "weapon_fire");

        //Fist
        registerFromAsset(context, "bHaptics/Weapon/Vest/Fist_Mirror.tact", PositionType.Left, "punch", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Fist_Mirror.tact", PositionType.ForearmL, "punch", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Fist.tact", PositionType.Right, "punch", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Fist.tact", PositionType.ForearmR, "punch", "weapon_fire");

        //Pistol
        registerFromAsset(context, "bHaptics/Weapon/Vest/Recoil_LV2_Mirror.tact", PositionType.Left, "pistol_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Recoil_LV2_Mirror.tact", PositionType.ForearmL, "pistol_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Recoil_LV2.tact", PositionType.Right, "pistol_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Recoil_LV2.tact", PositionType.ForearmR, "pistol_fire", "weapon_fire");

        //Shotgun
        registerFromAsset(context, "bHaptics/Weapon/Vest/Recoil_LV3_Mirror.tact", PositionType.Left, "shotgun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Recoil_LV3_Mirror.tact", PositionType.ForearmL, "shotgun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Recoil_LV3.tact", PositionType.Right, "shotgun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Recoil_LV3.tact", PositionType.ForearmR, "shotgun_fire", "weapon_fire");

        //Plasma Gun
        registerFromAsset(context, "bHaptics/Weapon/Vest/Recoil_LV1_Mirror.tact", PositionType.Left, "plasmagun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Recoil_LV1_Mirror.tact", PositionType.ForearmL, "plasmagun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Recoil_LV1.tact", PositionType.Right, "plasmagun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Recoil_LV1.tact", PositionType.ForearmR, "plasmagun_fire", "weapon_fire");

        //Grenade
        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Grenade_Init.tact", "handgrenade_init", "weapon_init");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Grenade_Throw.tact", "handgrenade_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Grenade_L.tact", PositionType.ForearmL, "handgrenade_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Grenade_R.tact", PositionType.ForearmR, "handgrenade_fire", "weapon_fire");

        //SMG
        registerFromAsset(context, "bHaptics/Weapon/Vest/Recoil_LV1_Mirror.tact", PositionType.Left, "machinegun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Recoil_LV1_Mirror.tact", PositionType.ForearmL, "machinegun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Recoil_LV1.tact", PositionType.Right, "machinegun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Recoil_LV1.tact", PositionType.ForearmR, "machinegun_fire", "weapon_fire");

        //Chaingun
        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Chaingun_Init.tact", "chaingun_init", "weapon_init");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Recoil_LV2_Mirror.tact", PositionType.Left, "chaingun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Recoil_LV2_Mirror.tact", PositionType.ForearmL, "chaingun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Recoil_LV2.tact", PositionType.Right, "chaingun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Recoil_LV2.tact", PositionType.ForearmR, "chaingun_fire", "weapon_fire");

        //BFG9000
        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_BFG9000_Init.tact", "bfg_init", "weapon_init");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Recoil_LV5_Mirror.tact", PositionType.Left, "bfg_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Recoil_LV5_Mirror.tact", PositionType.ForearmL, "bfg_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Recoil_LV5.tact", PositionType.Right, "bfg_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Recoil_LV5.tact", PositionType.ForearmR, "bfg_fire", "weapon_fire");

        //Rocket Launcher
        registerFromAsset(context, "bHaptics/Weapon/Vest/Recoil_LV4_Mirror.tact", PositionType.Left, "rocket_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Recoil_LV4_Mirror.tact", PositionType.ForearmL, "rocket_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Recoil_LV4.tact", PositionType.Right, "rocket_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Recoil_LV4.tact", PositionType.ForearmR, "rocket_fire", "weapon_fire");

        //Soul Cube
        registerFromAsset(context, "bHaptics/Weapon/Vest/SoulCube.tact", PositionType.Right, "soul_cube_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/SoulCube.tact", PositionType.ForearmR, "soul_cube_fire", "weapon_fire", 2.0f, 1.0f);
        registerFromAsset(context, "bHaptics/Weapon/Vest/SoulCube_Mirror.tact", PositionType.Left, "soul_cube_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/SoulCube_Mirror.tact", PositionType.ForearmL, "soul_cube_fire", "weapon_fire", 2.0f, 1.0f);

        initialised = true;
    }

    public static void registerFromAsset(Context context, String filename, PositionType type, String key, String group, float intensity, float duration)
    {
        String content = read(context, filename);
        if (content != null) {

            String hapticKey = key + "_" + type.name();
            player.registerProject(hapticKey, content);

            UUID uuid = UUID.randomUUID();
            Haptic haptic = new Haptic(type, hapticKey, uuid.toString(), group, intensity, duration);

            Vector<Haptic> haptics;
            if (!eventToEffectKeyMap.containsKey(key))
            {
                haptics = new Vector<>();
                haptics.add(haptic);
                eventToEffectKeyMap.put(key, haptics);
            }
            else
            {
                haptics = eventToEffectKeyMap.get(key);
                haptics.add(haptic);
            }
        }
    }

    public static void registerFromAsset(Context context, String filename, String key, String group)
    {
        registerFromAsset(context, filename, PositionType.Vest, key, group, 1.0f, 1.0f);
    }

    public static void registerFromAsset(Context context, String filename, PositionType type, String key, String group)
    {
        registerFromAsset(context, filename, type, key, group, 1.0f, 1.0f);
    }

    public static void destroy()
    {
        if (initialised) {
            BhapticsModule.destroy();
        }
    }

    private static boolean hasPermissions() {
        boolean blePermission = PermissionUtils.hasBluetoothPermission(context);
        boolean filePermission = PermissionUtils.hasFilePermissions(context);
        return blePermission && filePermission;
    }

    private static void requestPermissions() {
        if (!requestingPermission) {
            requestingPermission = true;
            ActivityCompat.requestPermissions((Activity) context,
                    new String[]{Manifest.permission.ACCESS_FINE_LOCATION},
                    2);
        }
    }

    private static boolean checkPermissionsAndInitialize() {
        if (hasPermissions()) {
            // Permissions have already been granted.
            return true;
        }
        else
        {
            requestPermissions();
        }

        return false;
    }

    public static void enable(Context ctx)
    {
        context = ctx;

        if (!enabled)
        {
            if (checkPermissionsAndInitialize()) {
                initialise();

                enabled = true;
            }
        }
    }

    public static void disable()
    {
        enabled = false;
        stopStreaming();
    }

    public static void startStreaming() {
        if (hapticStreamer == null) {
            hapticStreamer = new DefaultHapticStreamer();
            hapticStreamer.setCallback(new HapticStreamer.HapticStreamerCallback() {
                @Override
                public void onDiscover(String host) {
                    Log.i(TAG, "onDiscover: " + host);

                    hapticStreamer.connect(host);
                }

                @Override
                public void onConnect(String host) {

                }

                @Override
                public void onDisconnect(String host) {

                }
            });
            hapticStreamer.refreshCandidateIps();
        }
    }

    public static void stopStreaming() {
        if (hapticStreamer != null) {
            hapticStreamer.dispose();
            hapticStreamer = null;
        }
    }

    public static void refreshIp() {
        if (hapticStreamer != null) {
            hapticStreamer.refreshCandidateIps();
        }
    }

    public static void beginFrame()
    {
        if (enabled && hasPairedDevice) {
            repeatingHaptics.forEach((key, haptic) -> {
                //If a repeating haptic isn't playing, start it again
                if (!player.isPlaying(haptic.altKey)) {
                    player.submitRegistered(haptic.key, haptic.altKey, 100, 1.0f, 0, 0);
                }
            });
        }
        else
        {
            repeatingHaptics.clear();
        }
    }

    public static void endFrame() {}

    /*
       position values:
           0 - Will play on vest and both arms if tactosy tact files present for both
           1 - Will play on (left) vest and on left arm only if tactosy tact files present for left
           2 - Will play on (right) vest and on right arm only if tactosy tact files present for right
           3 - Will play on head only (if present)
           4 - Will play on all devices (that have a pattern defined for them)

       flag values:
           0 - No flags set
           1 - Indicate this is a looping effect that should play repeatedly until stopped

       intensity:
           0 - 100

       angle:
           0 - 360

       yHeight:
           -0.5 - 0.5
    */
    public static void playHaptic(String event, int position, int flags, float intensity, float angle, float yHeight)
    {
        if (enabled && hasPairedDevice) {
            String key = getHapticEventKey(event);

            //Log.v(TAG, event);

            //Special rumble effect that changes intensity per frame
            if (key.compareTo("rumble") == 0)
            {
                {
                    float highDuration = angle;

                    List<DotPoint> vector = new Vector<>();
                    int flipflop = 0;
                    for (int d = 0; d < 20; d += 4) // Only select every other dot
                    {
                        vector.add(new DotPoint(d + flipflop, (int) intensity));
                        vector.add(new DotPoint(d + 2 + flipflop, (int) intensity));
                        flipflop = 1 - flipflop;
                    }

                    player.submitDot("rumble_front", PositionType.VestFront, vector, (int) highDuration);
                    player.submitDot("rumble_back", PositionType.VestBack, vector, (int) highDuration);
                }
            }
            else if (eventToEffectKeyMap.containsKey(key)) {
                Vector<Haptic> haptics = eventToEffectKeyMap.get(key);
                for (Haptic haptic : haptics) {

                    //Don't allow a haptic to interrupt itself if it is already playing
                    if (player.isPlaying(haptics.get(0).altKey)) {
                        return;
                    }

                    //The following groups play at full intensity
                    if (haptic.group.compareTo("environment") == 0) {
                        intensity = 100;
                    }

                    {
                        float flIntensity = ((intensity / 100.0F) * haptic.intensity);
                        float flAngle = angle;
                        float flDuration = haptic.duration;

                        if (position > 0)
                        {
                            BhapticsManager manager = BhapticsModule.getBhapticsManager();

                            //If playing left position and haptic type is right, don;t play that one
                            if (position == 1)
                            {
                                if (haptic.type == PositionType.ForearmR ||
                                        haptic.type == PositionType.Right) {
                                    continue;
                                }
                            }

                            //If playing right position and haptic type is left, don;t play that one
                            if (position == 2)
                            {
                                if (haptic.type == PositionType.ForearmL ||
                                        haptic.type == PositionType.Left) {
                                    continue;
                                }
                            }

                            //Are we playing a "head only" pattern?
                            if (position == 3 &&
                                    (haptic.type != PositionType.Head || !manager.isDeviceConnected(BhapticsManager.DeviceType.Head)))
                            {
                                continue;
                            }

                            if (haptic.type == PositionType.Head) {
                                //Is this a "don't play on head" effect?
                                if (position < 3) {
                                    continue;
                                }

                                //Zero angle for head
                                flAngle = 0;
                            }
                        }

                        //Special values for heartbeat
                        if (haptic.group.compareTo("health") == 0)
                        {
                            //The worse condition we are in, the faster the heart beats!
                            float health = intensity;
                            flDuration = 1.0f - (0.4f * ((40 - health) / 40));
                            flIntensity = 1.0f;
                            flAngle = 0;
                        }

                        //If this is a repeating event, then add to the set to play in begin frame
                        if (flags == 1)
                        {
                            repeatingHaptics.put(key, haptic);
                        }
                        else {
                            player.submitRegistered(haptic.key, haptic.altKey, flIntensity, flDuration, flAngle, yHeight);
                        }
                    }
                }
            }
            else
            {
                Log.v(TAG, "Unknown Event: " + event);
            }
        }
    }

    private static String getHapticEventKey(String event) {
        String key = event.toLowerCase();
        if (key.contains("melee")) {
            if (key.contains("right"))
            {
                key = "melee_right";
            }
            else
            {
                key = "melee_left";
            }
        }
        else if (key.contains("damage")) {
            if (key.contains("bullet") ||
                    key.contains("splash") ||
                    key.contains("cgun")) {
                key = "bullet";
            }
            else if (key.contains("fireball") ||
                            key.contains("rocket") ||
                            key.contains("explode")) {
                key = "fireball"; // Just re-use this one
            }
            else if (key.contains("noair")) {
                key = "noair";
            }
            else if (key.contains("shotgun")) {
                key = "shotgun";
            }
            else if (key.contains("fall")) {
                key = "fall";
            }
        }
        else if (key.contains("door") || key.contains("panel") || key.contains("silver_sliding"))
        {
            key = "doorslide";
        }
        else if (key.contains("lift"))
        {
            if (key.contains("up"))
            {
                key = "liftup";
            }
            else if (key.contains("down"))
            {
                key = "liftdown";
            }
        }
        else if (key.contains("elevator"))
        {
            key = "machine";
        }
        else if (key.contains("entrance_scanner") || key.contains("scanner_rot1s"))
        {
            key = "scan";
        }
        else if (key.contains("decon_started"))
        {
            key = "decontaminate";
        }
        else if (key.contains("spark"))
        {
            key = "spark";
        }
        else if (key.contains("player") && key.contains("jump"))
        {
            key = "jump_start";
        }
        else if (key.contains("player") && key.contains("land"))
        {
            key = "jump_landing";
        }

        return key;
    }

    public static void stopHaptic(String event) {

        if (enabled && hasPairedDevice) {

            String key = getHapticEventKey(event);

            if (repeatingHaptics.containsKey(key))
            {
                Haptic haptic = repeatingHaptics.get(key);
                player.turnOff(haptic.altKey);

                repeatingHaptics.remove(key);
            }
        }
    }


    public static String read(Context context, String fileName) {
        try {
            InputStream is = context.getAssets().open(fileName);
            StringBuilder textBuilder = new StringBuilder();
            try (Reader reader = new BufferedReader(new InputStreamReader
                    (is, Charset.forName(StandardCharsets.UTF_8.name())))) {
                int c = 0;
                while ((c = reader.read()) != -1) {
                    textBuilder.append((char) c);
                }
            }

            return textBuilder.toString();
        } catch (IOException e) {
            Log.e(TAG, "read: ", e);
        }
        return null;
    }

    public static void scanIfNeeded() {
        BhapticsManager manager = BhapticsModule.getBhapticsManager();

        List<BhapticsDevice> deviceList = manager.getDeviceList();
        for (BhapticsDevice device : deviceList) {
            if (device.isPaired()) {
                hasPairedDevice = true;
                break;
            }
        }

        if (hasPairedDevice) {
            manager.scan();

            manager.addBhapticsManageCallback(new BhapticsManagerCallback() {
                @Override
                public void onDeviceUpdate(List<BhapticsDevice> list) {

                }

                @Override
                public void onScanStatusChange(boolean b) {

                }

                @Override
                public void onChangeResponse() {

                }

                @Override
                public void onConnect(String s) {
                    Thread t = new Thread(() -> {
                        try {
                            Thread.sleep(1000);
                            manager.ping(s);
                        }
                        catch (Throwable e) {
                        }
                    });
                    t.start();
                }

                @Override
                public void onDisconnect(String s) {

                }
            });

        }

        startStreaming();
    }
}
