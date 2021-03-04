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
import java.lang.reflect.Array;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.Vector;



public class bHaptics {

    public static enum HapticType {
        Vest,
        Tactosy_Left,
        Tactosy_Right
    };
    
    public static class Haptic
    {
        Haptic(HapticType type, String key, String altKey, float intensity, float duration) {
            this.type = type;
            this.key = key;
            this.altKey = altKey;
            this.intensity = intensity;
            this.duration = duration;
        }

        public final String key;
        public final String altKey;
        public final float intensity;
        public final float duration;
        public final HapticType type;
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
        registerFromAsset(context, "bHaptics/Damage/Body_Heartbeat.tact", HapticType.Vest, "heartbeat", "health", 1.0f, 1.2f);

        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Melee1.tact", "melee_left", "damage");
        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Melee2.tact", "melee_right", "damage");
        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Fireball.tact", "fireball", "damage");
        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Bullet.tact", "bullet", "damage");
        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Shotgun.tact", "shotgun", "damage");
        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Fire.tact", "fire", "damage");
        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Fire.tact", "noair", "damage"); // reuse
        registerFromAsset(context, "bHaptics/Damage/Body_DMG_Falling.tact", "fall", "damage");
        registerFromAsset(context, "bHaptics/Damage/Body_Shield_Break.tact", "shield_break", "damage");


        /*
            INTERACTIONS
         */
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Shield_Get.tact", "pickup_shield", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Pickup_L.tact", HapticType.Tactosy_Left, "pickup_shield", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Pickup_R.tact", HapticType.Tactosy_Right, "pickup_shield", "pickup");

        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Weapon_Get.tact", "pickup_weapon", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Pickup_L.tact", HapticType.Tactosy_Left, "pickup_weapon", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Pickup_R.tact", HapticType.Tactosy_Right, "pickup_weapon", "pickup");

        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Ammo_Get.tact", "pickup_ammo", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Ammo_L.tact", HapticType.Tactosy_Left, "pickup_ammo", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Ammo_R.tact", HapticType.Tactosy_Right, "pickup_ammo", "pickup");

        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Healstation.tact", "healstation", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Healthstation_L.tact", HapticType.Tactosy_Left, "healstation", "pickup");
        registerFromAsset(context, "bHaptics/Interaction/Arms/Healthstation_R.tact", HapticType.Tactosy_Right, "healstation", "pickup");

        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Door_Open.tact", "dooropen", "door");
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Door_Close.tact", "doorclose", "door");
        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Scan.tact", HapticType.Vest, "scan", "environment", 1.0f, 1.1f);
//        registerFromAsset(context, "bHaptics/Interaction/Vest/Body_Rumble.tact", "rumble", "rumble");
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
        registerFromAsset(context, "bHaptics/Weapon/Arms/Swap_L.tact", HapticType.Tactosy_Left, "weapon_switch", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Swap_R.tact", HapticType.Tactosy_Right, "weapon_switch", "weapon");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Reload.tact", "weapon_reload", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Reload_L.tact", HapticType.Tactosy_Left, "weapon_reload", "weapon");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Reload_R.tact", HapticType.Tactosy_Right, "weapon_reload", "weapon");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Punch_L.tact", "punchl", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Melee_L.tact", HapticType.Tactosy_Left, "punchl", "weapon_fire");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Punch_R.tact", "punchr", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Melee_R.tact", HapticType.Tactosy_Right, "punchr", "weapon_fire");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Pistol.tact", "pistol_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Pistol_L.tact", HapticType.Tactosy_Left, "pistol_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Pistol_R.tact", HapticType.Tactosy_Right, "pistol_fire", "weapon_fire");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Shotgun.tact", "shotgun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Shotgun_L.tact", HapticType.Tactosy_Left, "shotgun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Shotgun_R.tact", HapticType.Tactosy_Right, "shotgun_fire", "weapon_fire");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Plasmagun.tact", "plasmagun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/ShootDefault_L.tact", HapticType.Tactosy_Left, "plasmagun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/ShootDefault_R.tact", HapticType.Tactosy_Right, "plasmagun_fire", "weapon_fire");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Grenade_Init.tact", "handgrenade_init", "weapon_init");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Grenade_Throw.tact", "handgrenade_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Grenade_L.tact", HapticType.Tactosy_Left, "handgrenade_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Armd/Grenade_R.tact", HapticType.Tactosy_Right, "handgrenade_fire", "weapon_fire");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Machinegun.tact", "machinegun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/SMG_L.tact", HapticType.Tactosy_Left, "machinegun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/SMG_R.tact", HapticType.Tactosy_Right, "machinegun_fire", "weapon_fire");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Chaingun_Init.tact", "chaingun_init", "weapon_init");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_Chaingun_Fire.tact", "chaingun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Assault_L.tact", HapticType.Tactosy_Left, "chaingun_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/Assault_R.tact", HapticType.Tactosy_Right, "chaingun_fire", "weapon_fire");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_BFG9000_Init.tact", "bfg_init", "weapon_init");
        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_BFG9000_Fire.tact", "bfg_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/ShootDefault_L.tact", HapticType.Tactosy_Left, "bfg_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/ShootDefault_R.tact", HapticType.Tactosy_Right, "bfg_fire", "weapon_fire");

        registerFromAsset(context, "bHaptics/Weapon/Vest/Body_RocketLauncher.tact", "rocket_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/ShootDefault_L.tact", HapticType.Tactosy_Left, "rocket_fire", "weapon_fire");
        registerFromAsset(context, "bHaptics/Weapon/Arms/ShootDefault_R.tact", HapticType.Tactosy_Right, "rocket_fire", "weapon_fire");

        initialised = true;
    }

    public static void registerFromAsset(Context context, String filename, HapticType type, String key, String group, float intensity, float duration)
    {
        String content = read(context, filename);
        if (content != null) {

            String hapticKey = key + "_" + type.name();
            player.registerProject(hapticKey, content);

            Haptic haptic = new Haptic(type, hapticKey, group, intensity, duration);

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
        registerFromAsset(context, filename, HapticType.Vest, key, group, 1.0f, 1.0f);
    }

    public static void registerFromAsset(Context context, String filename, HapticType type, String key, String group)
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

    public static void playHaptic(String event, int position, float intensity, float angle, float yHeight)
    {
        if (enabled && hasPairedDevice) {
            String key = getHapticEventKey(event);

            Log.v(TAG, event);

            if (key.compareTo("rumble") == 0)
            {
                float highDuration = angle;

                List<DotPoint> vector = new Vector<>();
                for (int d = 0; d < 20; ++d)
                {
                    vector.add(new DotPoint(d, (int)intensity));
                }

                player.submitDot("rumble_front", PositionType.VestFront, vector, (int)highDuration);
                player.submitDot("rumble_back", PositionType.VestBack, vector, (int)highDuration);
            }
            else if (eventToEffectKeyMap.containsKey(key)) {
                Vector<Haptic> haptics = eventToEffectKeyMap.get(key);

                //Don't allow a haptic to interrupt itself if it is already playing
                if (player.isPlaying(haptics.get(0).altKey)) {
                    return;
                }

                for (Haptic haptic : haptics) {

                    //The following groups play at full intensity
                    if (haptic.altKey.compareTo("environment") == 0) {
                        intensity = 100;
                    }

                    if (position > 0)
                    {
                        //If playing left position and haptic type is right, don;t play that one
                        if (position == 1 && haptic.type == HapticType.Tactosy_Right)
                        {
                            continue;
                        }

                        //If playing right position and haptic type is left, don;t play that one
                        if (position == 2 && haptic.type == HapticType.Tactosy_Left)
                        {
                            continue;
                        }
                    }

                    if (haptic != null) {
                        float flIntensity = ((intensity / 100.0F) * haptic.intensity);
                        float duration = haptic.duration;

                        //Special hack for heartbeat
                        if (haptic.altKey.compareTo("health") == 0)
                        {
                            //The worse condition we are in, the faster the heart beats!
                            float health = intensity;
                            duration = 1.0f - (0.4f * ((40 - health) / 40));
                            flIntensity = 1.0f;
                        }

                        player.submitRegistered(haptic.key, haptic.altKey, flIntensity, duration, angle, yHeight);
                    }
                }
            }
        }
    }

    private static String getHapticEventKey(String event) {
        String key = event.toLowerCase();
        if (event.contains("melee")) {
            if (event.contains("right"))
            {
                key = "melee_right";
            }
            else
            {
                key = "melee_left";
            }
        } else if (event.contains("damage") && event.contains("bullet")) {
            key = "bullet";
        } else if (event.contains("damage") && event.contains("fireball")) {
            key = "fireball";
        } else if (event.contains("damage") && event.contains("noair")) {
            key = "noair";
        } else if (event.contains("damage") && event.contains("shotgun")) {
            key = "shotgun";
        } else if (event.contains("damage") && event.contains("fall")) {
            key = "fall";
        } else if (event.contains("door") || event.contains("panel"))
        {
            if (event.contains("close"))
            {
                key = "doorclose";
            }
            else if (event.contains("open"))
            {
                key = "dooropen";
            }
        } else if (event.contains("lift"))
        {
            if (event.contains("up"))
            {
                key = "liftup";
            }
            else if (event.contains("down"))
            {
                key = "liftdown";
            }
        } else if (event.contains("elevator"))
        {
            key = "machine";
        }  else if (event.contains("entrance_scanner") || event.contains("scanner_rot1s"))
        {
            key = "scan";
        }
        return key;
    }

    public static void stopAll() {

        if (hasPairedDevice) {
            player.turnOffAll();
        }
    }

    public static void stopHaptic(String event) {

        if (hasPairedDevice) {

            String key = getHapticEventKey(event);
            {
                player.turnOff(key);
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
