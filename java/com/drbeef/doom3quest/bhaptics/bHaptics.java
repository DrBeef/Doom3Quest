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
import java.util.Vector;



public class bHaptics {
    
    public static class Haptic
    {
        Haptic(String key, String altKey) {
            this.key = key;
            this.altKey = altKey;
        }
        public final String key;
        public final String altKey;
    };
    
    private static final String TAG = "Doom3Quest.bHaptics";

    private static Random rand = new Random();

    private static String currentEffect = "";

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
            return;
        }

        BhapticsModule.initialize(context);

        scanIfNeeded();

        player = BhapticsModule.getHapticPlayer();

        /*
            DAMAGE
        */
        Vector<Haptic> heartBeat = new Vector<>();
        heartBeat.add(registerFromAsset(context, "bHaptics/Damage/Body_Heartbeat.tact", "heartbeat", "heartbeat"));
        eventToEffectKeyMap.put("heartbeat", heartBeat);

        Vector<Haptic> damageMelee = new Vector<>();
        damageMelee.add(registerFromAsset(context, "bHaptics/Damage/Body_DMG_Melee1.tact", "melee1", "damage"));
        damageMelee.add(registerFromAsset(context, "bHaptics/Damage/Body_DMG_Melee2.tact", "melee2", "damage"));
        eventToEffectKeyMap.put("melee", damageMelee);

        Vector<Haptic> damageFireball = new Vector<>();
        damageFireball.add(registerFromAsset(context, "bHaptics/Damage/Body_DMG_Fireball.tact", "fireball", "damage"));
        eventToEffectKeyMap.put("fireball", damageFireball);

        Vector<Haptic> damageBullet = new Vector<>();
        damageBullet.add(registerFromAsset(context, "bHaptics/Damage/Body_DMG_Bullet.tact", "bullet", "damage"));
        eventToEffectKeyMap.put("bullet", damageBullet);

        Vector<Haptic> damageShotgun = new Vector<>();
        damageShotgun.add(registerFromAsset(context, "bHaptics/Damage/Body_DMG_Shotgun.tact", "shotgun", "damage"));
        eventToEffectKeyMap.put("shotgun", damageShotgun);

        Vector<Haptic> damageFire = new Vector<>();
        damageFire.add(registerFromAsset(context, "bHaptics/Damage/Body_DMG_Fire.tact", "fire", "damage"));
        eventToEffectKeyMap.put("fire", damageFire);

        Vector<Haptic> damageFall = new Vector<>();
        damageFall.add(registerFromAsset(context, "bHaptics/Damage/Body_DMG_Falling.tact", "fall", "damage"));
        eventToEffectKeyMap.put("fall", damageFall);

        Vector<Haptic> shieldBreak = new Vector<>();
        shieldBreak.add(registerFromAsset(context, "bHaptics/Damage/Body_Shield_Break.tact", "shield_break", "damage"));
        eventToEffectKeyMap.put("shield_break", shieldBreak);


        /*
            INTERACTIONS
         */
        Vector<Haptic> pickupShield = new Vector<>();
        pickupShield.add(registerFromAsset(context, "bHaptics/Interaction/Body_Shield_Get.tact", "pickup_shield", "pickup"));
        eventToEffectKeyMap.put("pickup_shield", pickupShield);

        //Need a tact file for this
//        Vector<KeyPair> pickup_weapon = new Vector<>();
//        pickup_weapon.add(registerFromAsset(context, "bHaptics/Interaction/Body_Weapon_Get.tact", "pickup_weapon", "pickup"));
//        eventToEffectKeyMap.put("pickup_weapon", pickup_weapon);

//        Vector<KeyPair> pickup_ammo = new Vector<>();
//        pickup_ammo.add(registerFromAsset(context, "bHaptics/Interaction/Body_Ammo_Get.tact", "pickup_ammo", "pickup"));
//        eventToEffectKeyMap.put("pickup_ammo", pickup_ammo);

        Vector<Haptic> healstation = new Vector<>();
        healstation.add(registerFromAsset(context, "bHaptics/Interaction/Body_Healstation.tact", "healstation", "pickup"));
        eventToEffectKeyMap.put("healstation", healstation);

        Vector<Haptic> doorOpen = new Vector<>();
        doorOpen.add(registerFromAsset(context, "bHaptics/Interaction/Body_Door_Open.tact", "dooropen", "door"));
        eventToEffectKeyMap.put("dooropen", doorOpen);

        Vector<Haptic> doorClose = new Vector<>();
        doorClose.add(registerFromAsset(context, "bHaptics/Interaction/Body_Door_Close.tact", "doorclose", "door"));
        eventToEffectKeyMap.put("doorclose", doorClose);

        Vector<Haptic> scan = new Vector<>();
        scan.add(registerFromAsset(context, "bHaptics/Interaction/Body_Scan.tact", "scan", "environment"));
        eventToEffectKeyMap.put("scan", scan);

        Vector<Haptic> rumble = new Vector<>();
        rumble.add(registerFromAsset(context, "bHaptics/Interaction/Body_Rumble.tact", "rumble", "environment"));
        eventToEffectKeyMap.put("rumble", rumble);

        Vector<Haptic> liftup = new Vector<>();
        liftup.add(registerFromAsset(context, "bHaptics/Interaction/Body_Chamber_Up.tact", "liftup", "environment"));
        eventToEffectKeyMap.put("liftup", liftup);

        Vector<Haptic> liftdown = new Vector<>();
        liftdown.add(registerFromAsset(context, "bHaptics/Interaction/Body_Chamber_Down.tact", "liftdown", "environment"));
        eventToEffectKeyMap.put("liftdown", liftdown);

        Vector<Haptic> machine = new Vector<>();
        machine.add(registerFromAsset(context, "bHaptics/Interaction/Body_Machine.tact", "machine", "environment"));
        eventToEffectKeyMap.put("machine", machine);


        /*
            WEAPONS
         */

        Vector<Haptic> weaponSwitch = new Vector<>();
        weaponSwitch.add(registerFromAsset(context, "bHaptics/Weapon/Body_Swap.tact", "weapon_switch", "weapon"));
        eventToEffectKeyMap.put("weapon_switch", weaponSwitch);

        Vector<Haptic> weaponReload = new Vector<>();
        weaponReload.add(registerFromAsset(context, "bHaptics/Weapon/Body_Reload.tact", "weapon_reload", "weapon"));
        eventToEffectKeyMap.put("weapon_reload", weaponReload);

        Vector<Haptic> punchL = new Vector<>();
        punchL.add(registerFromAsset(context, "bHaptics/Weapon/Body_Punch_L.tact", "punchL", "weapon"));
        eventToEffectKeyMap.put("punchL", punchL);

        Vector<Haptic> punchR = new Vector<>();
        punchR.add(registerFromAsset(context, "bHaptics/Weapon/Body_Punch_R.tact", "punchR", "weapon"));
        eventToEffectKeyMap.put("punchR", punchR);

        Vector<Haptic> weaponPistolFire = new Vector<>();
        weaponPistolFire.add(registerFromAsset(context, "bHaptics/Weapon/Body_Pistol.tact", "pistol_fire", "weapon_fire"));
        eventToEffectKeyMap.put("pistol_fire", weaponPistolFire);

        Vector<Haptic> weaponShotgunFire = new Vector<>();
        weaponShotgunFire.add(registerFromAsset(context, "bHaptics/Weapon/Body_Shotgun.tact", "shotgun_fire", "weapon_fire"));
        eventToEffectKeyMap.put("shotgun_fire", weaponShotgunFire);

        Vector<Haptic> weaponPlasmaFire = new Vector<>();
        weaponPlasmaFire.add(registerFromAsset(context, "bHaptics/Weapon/Body_Plasmagun.tact", "plasmagun_fire", "weapon_fire"));
        eventToEffectKeyMap.put("plasmagun_fire", weaponPlasmaFire);

        Vector<Haptic> weaponHandGrenadeInit = new Vector<>();
        weaponHandGrenadeInit.add(registerFromAsset(context, "bHaptics/Weapon/Body_Grenade_Init.tact", "handgrenade_init", "weapon_init"));
        eventToEffectKeyMap.put("handgrenade_init", weaponHandGrenadeInit);

        Vector<Haptic> weaponHandGrenadeFire = new Vector<>();
        weaponHandGrenadeFire.add(registerFromAsset(context, "bHaptics/Weapon/Body_Grenade_Throw.tact", "handgrenade_fire", "weapon_fire"));
        eventToEffectKeyMap.put("handgrenade_fire", weaponHandGrenadeFire);

        Vector<Haptic> weaponMachinegunFire = new Vector<>();
        weaponMachinegunFire.add(registerFromAsset(context, "bHaptics/Weapon/Body_Machinegun.tact", "machinegun_fire", "weapon_fire"));
        eventToEffectKeyMap.put("machinegun_fire", weaponMachinegunFire);

        Vector<Haptic> weaponChaingunInit = new Vector<>();
        weaponChaingunInit.add(registerFromAsset(context, "bHaptics/Weapon/Body_Chaingun_Init.tact", "chaingun_init", "weapon_init"));
        eventToEffectKeyMap.put("chaingun_init", weaponChaingunInit);

        Vector<Haptic> weaponChaingunFire = new Vector<>();
        weaponChaingunFire.add(registerFromAsset(context, "bHaptics/Weapon/Body_Chaingun_Fire.tact", "chaingun_fire", "weapon_fire"));
        eventToEffectKeyMap.put("chaingun_fire", weaponChaingunFire);

        Vector<Haptic> weaponBFGInit = new Vector<>();
        weaponBFGInit.add(registerFromAsset(context, "bHaptics/Weapon/Body_BFG9000_Init.tact", "bfg_init", "weapon_init"));
        eventToEffectKeyMap.put("bfg_init", weaponBFGInit);

        Vector<Haptic> weaponBFGFire = new Vector<>();
        weaponBFGFire.add(registerFromAsset(context, "bHaptics/Weapon/Body_BFG9000_Fire.tact", "bfg_fire", "weapon_fire"));
        eventToEffectKeyMap.put("bfg_fire", weaponBFGFire);

        Vector<Haptic> weaponRocketFire = new Vector<>();
        weaponRocketFire.add(registerFromAsset(context, "bHaptics/Weapon/Body_RocketLauncher.tact", "rocket_fire", "weapon_fire"));
        eventToEffectKeyMap.put("rocket_fire", weaponRocketFire);

        initialised = true;
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

    public static void playHaptic(String event, float intensity, float angle, float yHeight)
    {
        if (enabled && hasPairedDevice) {
            String key = getHapticEventKey(event);

            Log.v(TAG, event);

            if (eventToEffectKeyMap.containsKey(key)) {
                Vector<Haptic> haptics = eventToEffectKeyMap.get(key);

                Haptic haptic = haptics.get(rand.nextInt(haptics.size()));

                //If another haptic of this group is playing then
                if (player.isPlaying(haptic.altKey)) {
                    //we can't interrupt it
                    return;
                }

                //The following groups play at full intensity
                if (haptic.altKey.compareTo("environment") == 0)
                {
                    intensity = 100;
                }

                if (haptic != null) {
                    float flIntensity = (intensity / 100.0F);
                    player.submitRegistered(haptic.key, haptic.altKey, flIntensity, 1.0f, angle, yHeight);

                    currentEffect = key;
                }
            }
        }
    }

    private static String getHapticEventKey(String event) {
        String key = event;
        if (event.contains("melee")) {
            key = "melee";
        } else if (event.contains("damage") && event.contains("bullet")) {
            key = "bullet";
        } else if (event.contains("damage") && event.contains("fireball")) {
            key = "fireball";
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
            if (currentEffect.compareTo(key) == 0) {
                player.turnOff(key);
            }
        }
    }

    public static Haptic registerFromAsset(Context context, String filename, String key, String group)
    {
        String content = read(context, filename);
        if (content != null) {
            player.registerProject(key, content);
            return new Haptic(key, group);
        }
        return null;
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
                    manager.ping(s);
                }

                @Override
                public void onDisconnect(String s) {

                }
            });

            manager.scan();
        }
    }


}
