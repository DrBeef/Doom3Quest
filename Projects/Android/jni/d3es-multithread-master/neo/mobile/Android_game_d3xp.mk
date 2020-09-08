LOCAL_PATH := $(call my-dir)/../


include $(CLEAR_VARS)

LOCAL_MODULE := d3es_d3xp


LOCAL_C_INCLUDES :=  \
$(SDL_INCLUDE_PATHS) \
$(TOP_DIR)/Doom/d3es/neo/mobile \
$(TOP_DIR)/Doom/d3es/neo/d3xp \


LOCAL_CPPFLAGS :=  -DGAME_DLL -D_D3XP -DCTF -fPIC

LOCAL_CPPFLAGS += -std=gnu++11 -D__DOOM_DLL__ -frtti -fexceptions  -Wno-error=format-security


LOCAL_CPPFLAGS += -Wno-sign-compare \
                  -Wno-switch \
                  -Wno-format-security \


# Not avaliable in Android untill N
LOCAL_CFLAGS := -DIOAPI_NO_64

LOCAL_CFLAGS +=  -fno-unsafe-math-optimizations -fno-strict-aliasing -fno-math-errno -fno-trapping-math -fsigned-char


src_idlib = \
	idlib/bv/Bounds.cpp \
	idlib/bv/Frustum.cpp \
	idlib/bv/Sphere.cpp \
	idlib/bv/Box.cpp \
	idlib/geometry/DrawVert.cpp \
	idlib/geometry/Winding2D.cpp \
	idlib/geometry/Surface_SweptSpline.cpp \
	idlib/geometry/Winding.cpp \
	idlib/geometry/Surface.cpp \
	idlib/geometry/Surface_Patch.cpp \
	idlib/geometry/TraceModel.cpp \
	idlib/geometry/JointTransform.cpp \
	idlib/hashing/CRC32.cpp \
	idlib/hashing/MD4.cpp \
	idlib/hashing/MD5.cpp \
	idlib/math/Angles.cpp \
	idlib/math/Lcp.cpp \
	idlib/math/Math.cpp \
	idlib/math/Matrix.cpp \
	idlib/math/Ode.cpp \
	idlib/math/Plane.cpp \
	idlib/math/Pluecker.cpp \
	idlib/math/Polynomial.cpp \
	idlib/math/Quat.cpp \
	idlib/math/Rotation.cpp \
	idlib/math/Simd.cpp \
	idlib/math/Simd_Generic.cpp \
	idlib/math/Simd_AltiVec.cpp \
	idlib/math/Simd_MMX.cpp \
	idlib/math/Simd_3DNow.cpp \
	idlib/math/Simd_SSE.cpp \
	idlib/math/Simd_SSE2.cpp \
	idlib/math/Simd_SSE3.cpp \
	idlib/math/Vector.cpp \
	idlib/BitMsg.cpp \
	idlib/LangDict.cpp \
	idlib/Lexer.cpp \
	idlib/Lib.cpp \
	idlib/containers/HashIndex.cpp \
	idlib/Dict.cpp \
	idlib/Str.cpp \
	idlib/Parser.cpp \
	idlib/MapFile.cpp \
	idlib/CmdArgs.cpp \
	idlib/Token.cpp \
	idlib/Base64.cpp \
	idlib/Timer.cpp \
	idlib/Heap.cpp \


src_game = \
    d3xp/AF.cpp \
	d3xp/AFEntity.cpp \
	d3xp/Actor.cpp \
	d3xp/Camera.cpp \
	d3xp/Entity.cpp \
	d3xp/BrittleFracture.cpp \
	d3xp/Fx.cpp \
	d3xp/GameEdit.cpp \
	d3xp/Game_local.cpp \
	d3xp/Game_network.cpp \
	d3xp/Item.cpp \
	d3xp/IK.cpp \
	d3xp/Light.cpp \
	d3xp/Misc.cpp \
	d3xp/Mover.cpp \
	d3xp/Moveable.cpp \
	d3xp/MultiplayerGame.cpp \
	d3xp/Player.cpp \
	d3xp/PlayerIcon.cpp \
	d3xp/PlayerView.cpp \
	d3xp/Projectile.cpp \
	d3xp/Pvs.cpp \
	d3xp/SecurityCamera.cpp \
	d3xp/SmokeParticles.cpp \
	d3xp/Sound.cpp \
	d3xp/Target.cpp \
	d3xp/Trigger.cpp \
	d3xp/Weapon.cpp \
	d3xp/WorldSpawn.cpp \
	d3xp/ai/AAS.cpp \
	d3xp/ai/AAS_debug.cpp \
	d3xp/ai/AAS_pathing.cpp \
	d3xp/ai/AAS_routing.cpp \
	d3xp/ai/AI.cpp \
	d3xp/ai/AI_events.cpp \
	d3xp/ai/AI_pathing.cpp \
	d3xp/ai/AI_Vagary.cpp \
	d3xp/gamesys/DebugGraph.cpp \
	d3xp/gamesys/Class.cpp \
	d3xp/gamesys/Event.cpp \
	d3xp/gamesys/SaveGame.cpp \
	d3xp/gamesys/SysCmds.cpp \
	d3xp/gamesys/SysCvar.cpp \
	d3xp/gamesys/TypeInfo.cpp \
	d3xp/anim/Anim.cpp \
	d3xp/anim/Anim_Blend.cpp \
	d3xp/anim/Anim_Import.cpp \
	d3xp/anim/Anim_Testmodel.cpp \
	d3xp/script/Script_Compiler.cpp \
	d3xp/script/Script_Interpreter.cpp \
	d3xp/script/Script_Program.cpp \
	d3xp/script/Script_Thread.cpp \
	d3xp/physics/Clip.cpp \
	d3xp/physics/Force.cpp \
	d3xp/physics/Force_Constant.cpp \
	d3xp/physics/Force_Drag.cpp \
	d3xp/physics/Force_Field.cpp \
	d3xp/physics/Force_Spring.cpp \
	d3xp/physics/Physics.cpp \
	d3xp/physics/Physics_AF.cpp \
	d3xp/physics/Physics_Actor.cpp \
	d3xp/physics/Physics_Base.cpp \
	d3xp/physics/Physics_Monster.cpp \
	d3xp/physics/Physics_Parametric.cpp \
	d3xp/physics/Physics_Player.cpp \
	d3xp/physics/Physics_RigidBody.cpp \
	d3xp/physics/Physics_Static.cpp \
	d3xp/physics/Physics_StaticMulti.cpp \
	d3xp/physics/Push.cpp \
	d3xp/Grabber.cpp \
	d3xp/physics/Force_Grab.cpp \

LOCAL_SRC_FILES = $(src_idlib) $(src_game)

LOCAL_SHARED_LIBRARIES := saffal
LOCAL_STATIC_LIBRARIES :=
LOCAL_LDLIBS :=

include $(BUILD_SHARED_LIBRARY)
