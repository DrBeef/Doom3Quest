package com.drbeef.doom3quest.bhaptics;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.support.v4.app.ActivityCompat;
import android.util.Log;

import com.bhaptics.bhapticsmanger.BhapticsManager;
import com.bhaptics.bhapticsmanger.BhapticsManagerCallback;
import com.bhaptics.bhapticsmanger.BhapticsModule;
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
        registerFromAsset(context, "bHaptics/Damage/Head_DMG_Default.tact", PositionType.Head, "melee_left", "damage");

        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Melee2.tact", "melee_right", "damage");
        registerFromAsset(context, "bHaptics/Damage/Head_DMG_Default.tact", PositionType.Head, "melee_right", "damage");

        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Fireball.tact", "fireball", "damage");
        registerFromAsset(context, "bHaptics/Damage/Head_DMG_Default.tact", PositionType.Head, "fireball", "damage");

        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Bullet.tact", "bullet", "damage");
        registerFromAsset(context, "bHaptics/Damage/Head_DMG_Default.tact", PositionType.Head, "bullet", "damage");

        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Shotgun.tact", "shotgun", "damage");
        registerFromAsset(context, "bHaptics/Damage/Head_DMG_Default.tact", PositionType.Head, "shotgun", "damage");

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
        registerFromAsset(context, "bHaptics/Interaction/Arms/Pickup_L.tact", PositionType.ForearmL, "pickup_weapon", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Pickup_R.tact", PositionType.ForearmR, "pickup_weapon", "pickup");

        //registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Ammo_Get.tact", "pickup_ammo", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Ammo_L.tact", PositionType.ForearmL, "pickup_ammo", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Ammo_R.tact", PositionType.ForearmR, "pickup_ammo", "pickup");

        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Healstation.tact", "healstation", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Healthstation_L.tact", PositionType.ForearmL, "healstation", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Healthstation_R.tact", PositionType.ForearmR, "healstation", "pickup");

        //These are when the game plays the heartbeat pulse sound, which is not triggered by low health
        registerFromAsset(context, "bHaptics/Damage/Body_Heartbeat.tact", PositionType.Vest, "heartbeat_pulse_loop", "heart_sound1", 0.6f, 1.4f);
        registerFromAsset(context, "bHaptics/Damage/Body_Heartbeat.tact", PositionType.Vest, "heartbeat4", "heart_sound2", 0.6f, 1.4f);

        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Door_Open.tact", "dooropen", "door");
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Door_Close.tact", "doorclose", "door");
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Scan.tact", PositionType.Vest, "scan", "environment", 1.0f, 1.15f);
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Scan.tact", PositionType.Vest, "decontaminate", "environment", 0.5f, 0.65f);
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Chamber_Up.tact", "liftup", "environment");
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Chamber_Down.tact", "liftdown", "environment");
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Machine.tact", "machine", "environment");

        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_PDA_Open.tact", "pda_open", "pda");
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_PDA_Open.tact", "pda_close", "pda");
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_PDA_Alarm.tact", "pda_alarm", "pda");
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_PDA_Touch.tact", "pda_touch", "pda");


        /*
            WEAPONS
         */

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Swap.tact", "weapon_switch", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Swap_L.tact", PositionType.ForearmL, "weapon_switch", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Swap_R.tact", PositionType.ForearmR, "weapon_switch", "weapon");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Reload.tact", "weapon_reload", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Reload_L.tact", PositionType.ForearmL, "weapon_reload", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Reload_R.tact", PositionType.ForearmR, "weapon_reload", "weapon");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Punch_L.tact", "punch_left", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Melee_L.tact", PositionType.ForearmL, "punch_left", "weapon_fire");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Punch_R.tact", "punch_right", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Melee_R.tact", PositionType.ForearmR, "punch_right", "weapon_fire");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Pistol.tact", "pistol_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Pistol_L.tact", PositionType.ForearmL, "pistol_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Pistol_R.tact", PositionType.ForearmR, "pistol_fire", "weapon_fire");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Shotgun.tact", "shotgun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Shotgun_L.tact", PositionType.ForearmL, "shotgun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Shotgun_R.tact", PositionType.ForearmR, "shotgun_fire", "weapon_fire");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Plasmagun.tact", "plasmagun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Assault_L.tact", PositionType.ForearmL, "plasmagun_fire", "weapon_fire", 0.8f, 0.5f);
        registerFromAsset(context, "bHaptics/Weapon/Arms/Assault_R.tact", PositionType.ForearmR, "plasmagun_fire", "weapon_fire", 0.8f, 0.5f);

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Grenade_Init.tact", "handgrenade_init", "weapon_init");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Grenade_Throw.tact", "handgrenade_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Grenade_L.tact", PositionType.ForearmL, "handgrenade_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Grenade_R.tact", PositionType.ForearmR, "handgrenade_fire", "weapon_fire");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Machinegun.tact", "machinegun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/SMG_L.tact", PositionType.ForearmL, "machinegun_fire", "weapon_fire", 0.9f, 0.8f);
        registerFromAsset(context, "bHaptics/Weapon/Arms/SMG_R.tact", PositionType.ForearmR, "machinegun_fire", "weapon_fire", 0.9f, 0.8f);

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Chaingun_Init.tact", "chaingun_init", "weapon_init");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Chaingun_Fire.tact", "chaingun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/SMG_L.tact", PositionType.ForearmL, "chaingun_fire", "weapon_fire", 1.4f, 0.8f);
        registerFromAsset(context, "bHaptics/Weapon/Arms/SMG_R.tact", PositionType.ForearmR, "chaingun_fire", "weapon_fire", 1.4f, 0.8f);

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_BFG9000_Init.tact", "bfg_init", "weapon_init");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_BFG9000_Fire.tact", "bfg_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/ShootDefault_L.tact", PositionType.ForearmL, "bfg_fire", "weapon_fire", 2.0f, 2.0f);
        registerFromAsset(context, "bHaptics/Weapon/Arms/ShootDefault_R.tact", PositionType.ForearmR, "bfg_fire", "weapon_fire", 2.0f, 2.0f);

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_RocketLauncher.tact", "rocket_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/ShootDefault_L.tact", PositionType.ForearmL, "rocket_fire", "weapon_fire", 2.0f, 1.0f);
        registerFromAsset(context, "bHaptics/Weapon/Arms/ShootDefault_R.tact", PositionType.ForearmR, "rocket_fire", "weapon_fire", 2.0f, 1.0f);

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
           1 - Will play on vest and on left arm only if tactosy tact files present for left
           2 - Will play on vest and on right arm only if tactosy tact files present for right
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
                            if (position == 1 && haptic.type == PositionType.ForearmR)
                            {
                                continue;
                            }

                            //If playing right position and haptic type is left, don;t play that one
                            if (position == 2 && haptic.type == PositionType.ForearmL)
                            {
                                continue;
                            }


                            if (position == 3 &&
                                    (haptic.type != PositionType.Head || !manager.isDeviceConnected(BhapticsManager.DeviceType.Head)))
                            {
                                continue;
                            }

                            if (haptic.type == PositionType.Head) {
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
                //Log.v(TAG, "Unknown Event: " + event);
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
        } else if (key.contains("damage")) {
            if (key.contains("bullet") ||
                    key.contains("splash") ||
                    key.contains("cgun")) {
                key = "bullet";
            } else if (key.contains("fireball") ||
                            key.contains("rocket") ||
                            key.contains("explode")) {
                key = "fireball"; // Just re-use this one
            } else if (key.contains("noair")) {
                key = "noair";
            } else if (key.contains("shotgun")) {
                key = "shotgun";
            } else if (key.contains("fall")) {
                key = "fall";
            }
        } else if (key.contains("door") || key.contains("panel"))
        {
            if (key.contains("close"))
            {
                key = "doorclose";
            }
            else if (key.contains("open"))
            {
                key = "dooropen";
            }
        } else if (key.contains("lift"))
        {
            if (key.contains("up"))
            {
                key = "liftup";
            }
            else if (key.contains("down"))
            {
                key = "liftdown";
            }
        } else if (key.contains("elevator"))
        {
            key = "machine";
        }  else if (key.contains("entrance_scanner") || key.contains("scanner_rot1s"))
        {
            key = "scan";
        }  else if (key.contains("decon_started"))
        {
            key = "decontaminate";
        }
        return key;
    }

    public static void stopHaptic(String event) {

        if (enabled && hasPairedDevice) {

            String key = getHapticEventKey(event);

            if (repeatingHaptics.containsKey(key))
            {
                player.turnOff(key);

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
    }
}
