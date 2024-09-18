LOCAL_PATH := $(call my-dir)/../


include $(CLEAR_VARS)

LOCAL_MODULE := d3es_game

LOCAL_C_INCLUDES :=  \
$(SDL_INCLUDE_PATHS) \
$(D3QUEST_TOP_PATH)/neo/mobile \
$(D3QUEST_TOP_PATH)/neo/game \
$(SDL_INCLUDE_PATHS)


LOCAL_CPPFLAGS :=  -DGAME_DLL -fPIC

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
	game/AF.cpp \
	game/AFEntity.cpp \
	game/Actor.cpp \
	game/Camera.cpp \
	game/Entity.cpp \
	game/BrittleFracture.cpp \
	game/Fx.cpp \
	game/GameEdit.cpp \
	game/Game_local.cpp \
	game/Game_network.cpp \
	game/Grabber.cpp \
	game/Item.cpp \
	game/IK.cpp \
	game/Light.cpp \
	game/Misc.cpp \
	game/Mover.cpp \
	game/Moveable.cpp \
	game/MultiplayerGame.cpp \
	game/Player.cpp \
	game/PlayerIcon.cpp \
	game/PlayerView.cpp \
	game/Projectile.cpp \
	game/Pvs.cpp \
	game/SecurityCamera.cpp \
	game/SmokeParticles.cpp \
	game/Sound.cpp \
	game/Target.cpp \
	game/Trigger.cpp \
	game/Vr.cpp \
	game/Weapon.cpp \
	game/WorldSpawn.cpp \
	game/ai/AAS.cpp \
	game/ai/AAS_debug.cpp \
	game/ai/AAS_pathing.cpp \
	game/ai/AAS_routing.cpp \
	game/ai/AI.cpp \
	game/ai/AI_events.cpp \
	game/ai/AI_pathing.cpp \
	game/ai/AI_Vagary.cpp \
	game/gamesys/DebugGraph.cpp \
	game/gamesys/Class.cpp \
	game/gamesys/Event.cpp \
	game/gamesys/SaveGame.cpp \
	game/gamesys/SysCmds.cpp \
	game/gamesys/SysCvar.cpp \
	game/gamesys/TypeInfo.cpp \
	game/anim/Anim.cpp \
	game/anim/Anim_Blend.cpp \
	game/anim/Anim_Import.cpp \
	game/anim/Anim_Testmodel.cpp \
	game/script/Script_Compiler.cpp \
	game/script/Script_Interpreter.cpp \
	game/script/Script_Program.cpp \
	game/script/Script_Thread.cpp \
	game/physics/Clip.cpp \
	game/physics/Force.cpp \
	game/physics/Force_Constant.cpp \
	game/physics/Force_Drag.cpp \
	game/physics/Force_Field.cpp \
	game/physics/Force_Grab.cpp \
	game/physics/Force_Spring.cpp \
	game/physics/Physics.cpp \
	game/physics/Physics_AF.cpp \
	game/physics/Physics_Actor.cpp \
	game/physics/Physics_Base.cpp \
	game/physics/Physics_Monster.cpp \
	game/physics/Physics_Parametric.cpp \
	game/physics/Physics_Player.cpp \
	game/physics/Physics_RigidBody.cpp \
	game/physics/Physics_Static.cpp \
	game/physics/Physics_StaticMulti.cpp \
	game/physics/Push.cpp \

LOCAL_SRC_FILES = $(src_idlib) $(src_game)

LOCAL_SHARED_LIBRARIES := 
LOCAL_STATIC_LIBRARIES :=
LOCAL_LDLIBS :=

include $(BUILD_SHARED_LIBRARY)
