GPP = g++
CC = gcc
LD = ld

LIBHX86_INCLUDE ?= ../Hashx86-OS/user_prog/libhx86/include
LIBMBEDTLS_INCLUDE ?= ../Hashx86-OS/user_prog/libmbedtls/include
MBEDTLS_INCLUDE ?= ../Hashx86-OS/third_party/mbedtls/include
KERNEL_INCLUDE ?= ../Hashx86-OS/include
LIBHX86_LIB_DIR ?= ../Hashx86-OS/build/libhx86
LIBMBEDTLS_LIB_DIR ?= ../Hashx86-OS/build/libmbedtls

INCLUDES = -Iinclude -I. -I$(LIBHX86_INCLUDE) -I$(LIBMBEDTLS_INCLUDE) \
           -I$(MBEDTLS_INCLUDE) -I$(KERNEL_INCLUDE)

BASE_CFLAGS = -m32 -O2 -g -ffreestanding -nostdlib -fno-builtin -fno-strict-aliasing -fcommon -fomit-frame-pointer -fpermissive -Wno-write-strings -Wno-return-type -DREF_HARD_LINKED -DC_ONLY -U__linux__ -Ulinux -U__linux $(INCLUDES)
BASE_CXXFLAGS = $(BASE_CFLAGS) -fno-rtti -fno-exceptions

ARCHIVE_INCLUDES = -Idata/compat -Iinclude -I$(LIBHX86_INCLUDE) -I$(KERNEL_INCLUDE) \
                   -I../third_party/miniz
ARCHIVE_CFLAGS = -m32 -O2 -g -ffreestanding -nostdlib -fno-builtin -fno-common \
                 -fno-omit-frame-pointer -MMD -MP \
                 -DMINIZ_NO_STDIO -DMINIZ_NO_DEFLATE_APIS -DMINIZ_NO_TIME -DNDEBUG \
                 $(ARCHIVE_INCLUDES)

ENGINE_DIR = engine
ENGINE_OBJ_DIR = ../build/obj/quake2
BUILD_DIR = ../build

# Source files grouped by directory
CLIENT_SRC = \
	$(ENGINE_DIR)/client/cl_cin.c \
	$(ENGINE_DIR)/client/cl_ents.c \
	$(ENGINE_DIR)/client/cl_fx.c \
	$(ENGINE_DIR)/client/cl_input.c \
	$(ENGINE_DIR)/client/cl_inv.c \
	$(ENGINE_DIR)/client/cl_main.c \
	$(ENGINE_DIR)/client/cl_newfx.c \
	$(ENGINE_DIR)/client/cl_parse.c \
	$(ENGINE_DIR)/client/cl_pred.c \
	$(ENGINE_DIR)/client/cl_scrn.c \
	$(ENGINE_DIR)/client/cl_tent.c \
	$(ENGINE_DIR)/client/cl_view.c \
	$(ENGINE_DIR)/client/console.c \
	$(ENGINE_DIR)/client/keys.c \
	$(ENGINE_DIR)/client/menu.c \
	$(ENGINE_DIR)/client/qmenu.c \
	$(ENGINE_DIR)/client/snd_dma.c \
	$(ENGINE_DIR)/client/snd_mem.c \
	$(ENGINE_DIR)/client/snd_mix.c

SERVER_SRC = \
	$(ENGINE_DIR)/server/sv_ccmds.c \
	$(ENGINE_DIR)/server/sv_ents.c \
	$(ENGINE_DIR)/server/sv_game.c \
	$(ENGINE_DIR)/server/sv_init.c \
	$(ENGINE_DIR)/server/sv_main.c \
	$(ENGINE_DIR)/server/sv_send.c \
	$(ENGINE_DIR)/server/sv_user.c \
	$(ENGINE_DIR)/server/sv_world.c

QCOMMON_SRC = \
	$(ENGINE_DIR)/qcommon/cmd.c \
	$(ENGINE_DIR)/qcommon/cmodel.c \
	$(ENGINE_DIR)/qcommon/common.c \
	$(ENGINE_DIR)/qcommon/crc.c \
	$(ENGINE_DIR)/qcommon/cvar.c \
	$(ENGINE_DIR)/qcommon/files.c \
	$(ENGINE_DIR)/qcommon/md4.c \
	$(ENGINE_DIR)/qcommon/net_chan.c \
	$(ENGINE_DIR)/qcommon/pmove.c

PLATFORM_SRC = \
	sys_hx86.c \
	vid_hx86.c \
	swimp_hx86.c \
	net_hx86.c \
	q_shhx86.c \
	compat.c \
	prog.cpp

DATA_SRC = \
	data/Archive.cpp \
	data/ArchivePort.cpp \
	data/HttpClient.cpp \
	data/StoreUtil.cpp

ARCHIVE_C = \
	../third_party/miniz/miniz.c \
	../third_party/miniz/miniz_tinfl.c \
	../third_party/miniz/miniz_zip.c

NULL_SRC = \
	$(ENGINE_DIR)/null/cd_null.c \
	$(ENGINE_DIR)/null/snddma_hx86.c

# Game module (statically linked)
GAME_SRC = \
	$(ENGINE_DIR)/game/g_ai.c \
	$(ENGINE_DIR)/game/g_cmds.c \
	$(ENGINE_DIR)/game/g_combat.c \
	$(ENGINE_DIR)/game/g_func.c \
	$(ENGINE_DIR)/game/g_items.c \
	$(ENGINE_DIR)/game/g_main.c \
	$(ENGINE_DIR)/game/g_misc.c \
	$(ENGINE_DIR)/game/g_monster.c \
	$(ENGINE_DIR)/game/g_phys.c \
	$(ENGINE_DIR)/game/g_save.c \
	$(ENGINE_DIR)/game/g_spawn.c \
	$(ENGINE_DIR)/game/g_svcmds.c \
	$(ENGINE_DIR)/game/g_target.c \
	$(ENGINE_DIR)/game/g_trigger.c \
	$(ENGINE_DIR)/game/g_turret.c \
	$(ENGINE_DIR)/game/g_utils.c \
	$(ENGINE_DIR)/game/g_weapon.c \
	$(ENGINE_DIR)/game/g_chase.c \
	$(ENGINE_DIR)/game/m_actor.c \
	$(ENGINE_DIR)/game/m_berserk.c \
	$(ENGINE_DIR)/game/m_boss2.c \
	$(ENGINE_DIR)/game/m_boss3.c \
	$(ENGINE_DIR)/game/m_boss31.c \
	$(ENGINE_DIR)/game/m_boss32.c \
	$(ENGINE_DIR)/game/m_brain.c \
	$(ENGINE_DIR)/game/m_chick.c \
	$(ENGINE_DIR)/game/m_flash.c \
	$(ENGINE_DIR)/game/m_flipper.c \
	$(ENGINE_DIR)/game/m_float.c \
	$(ENGINE_DIR)/game/m_flyer.c \
	$(ENGINE_DIR)/game/m_gladiator.c \
	$(ENGINE_DIR)/game/m_gunner.c \
	$(ENGINE_DIR)/game/m_hover.c \
	$(ENGINE_DIR)/game/m_infantry.c \
	$(ENGINE_DIR)/game/m_insane.c \
	$(ENGINE_DIR)/game/m_medic.c \
	$(ENGINE_DIR)/game/m_move.c \
	$(ENGINE_DIR)/game/m_mutant.c \
	$(ENGINE_DIR)/game/m_parasite.c \
	$(ENGINE_DIR)/game/m_soldier.c \
	$(ENGINE_DIR)/game/m_supertank.c \
	$(ENGINE_DIR)/game/m_tank.c \
	$(ENGINE_DIR)/game/p_client.c \
	$(ENGINE_DIR)/game/p_hud.c \
	$(ENGINE_DIR)/game/p_trail.c \
	$(ENGINE_DIR)/game/p_view.c \
	$(ENGINE_DIR)/game/p_weapon.c \
	$(ENGINE_DIR)/game/q_shared.c

# Software renderer (statically linked)
REF_SOFT_SRC = \
	$(ENGINE_DIR)/ref_soft/r_aclip.c \
	$(ENGINE_DIR)/ref_soft/r_alias.c \
	$(ENGINE_DIR)/ref_soft/r_bsp.c \
	$(ENGINE_DIR)/ref_soft/r_draw.c \
	$(ENGINE_DIR)/ref_soft/r_edge.c \
	$(ENGINE_DIR)/ref_soft/r_image.c \
	$(ENGINE_DIR)/ref_soft/r_light.c \
	$(ENGINE_DIR)/ref_soft/r_main.c \
	$(ENGINE_DIR)/ref_soft/r_misc.c \
	$(ENGINE_DIR)/ref_soft/r_model.c \
	$(ENGINE_DIR)/ref_soft/r_part.c \
	$(ENGINE_DIR)/ref_soft/r_poly.c \
	$(ENGINE_DIR)/ref_soft/r_polyse.c \
	$(ENGINE_DIR)/ref_soft/r_rast.c \
	$(ENGINE_DIR)/ref_soft/r_scan.c \
	$(ENGINE_DIR)/ref_soft/r_sprite.c \
	$(ENGINE_DIR)/ref_soft/r_surf.c

ALL_C_SRC = $(CLIENT_SRC) $(SERVER_SRC) $(QCOMMON_SRC) $(PLATFORM_SRC) $(NULL_SRC) $(GAME_SRC) $(REF_SOFT_SRC)
ALL_OBJS = $(patsubst %.cpp,$(ENGINE_OBJ_DIR)/%.o,$(patsubst %.c,$(ENGINE_OBJ_DIR)/%.o,$(ALL_C_SRC)))
ALL_OBJS += $(addprefix $(ENGINE_OBJ_DIR)/, $(DATA_SRC:.cpp=.o))
ARCHIVE_OBJS = $(addprefix $(ENGINE_OBJ_DIR)/archive/, $(notdir $(ARCHIVE_C:.c=.o)))
ALL_OBJS += $(ARCHIVE_OBJS)
TARGET = $(BUILD_DIR)/Quake2.bin

.PHONY: all clean

all: $(TARGET)

# Compile .c files
$(ENGINE_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(GPP) $(BASE_CFLAGS) -Iengine/client -Iengine/server -Iengine/qcommon -Iengine/game -Iengine/ref_soft -Iengine/null -o $@ -c $<

# Compile .cpp files
$(ENGINE_OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(GPP) $(BASE_CXXFLAGS) -Iengine/client -Iengine/server -Iengine/qcommon -Iengine/game -Iengine/ref_soft -Iengine/null -o $@ -c $<

# App data/downloader sources need the archive and mbedTLS include paths.
$(ENGINE_OBJ_DIR)/data/%.o: data/%.cpp
	@mkdir -p $(dir $@)
	$(GPP) $(BASE_CXXFLAGS) -Idata/compat -I../third_party/miniz -MMD -MP -o $@ -c $<

$(ENGINE_OBJ_DIR)/archive/%.o: ../third_party/miniz/%.c
	@mkdir -p $(dir $@)
	$(CC) $(ARCHIVE_CFLAGS) -o $@ -c $<

# Engine source in subdirs needs include paths adjusted
$(ENGINE_OBJ_DIR)/engine/client/%.o: $(ENGINE_DIR)/client/%.c
	@mkdir -p $(dir $@)
	$(GPP) $(BASE_CFLAGS) -I$(ENGINE_DIR)/client -o $@ -c $<

$(ENGINE_OBJ_DIR)/engine/server/%.o: $(ENGINE_DIR)/server/%.c
	@mkdir -p $(dir $@)
	$(GPP) $(BASE_CFLAGS) -I$(ENGINE_DIR)/server -o $@ -c $<

$(ENGINE_OBJ_DIR)/engine/qcommon/%.o: $(ENGINE_DIR)/qcommon/%.c
	@mkdir -p $(dir $@)
	$(GPP) $(BASE_CFLAGS) -I$(ENGINE_DIR)/qcommon -o $@ -c $<

$(ENGINE_OBJ_DIR)/engine/game/%.o: $(ENGINE_DIR)/game/%.c
	@mkdir -p $(dir $@)
	$(GPP) $(BASE_CFLAGS) -I$(ENGINE_DIR)/game -o $@ -c $<

$(ENGINE_OBJ_DIR)/engine/ref_soft/%.o: $(ENGINE_DIR)/ref_soft/%.c
	@mkdir -p $(dir $@)
	$(GPP) $(BASE_CFLAGS) -I$(ENGINE_DIR)/ref_soft -o $@ -c $<

$(ENGINE_OBJ_DIR)/engine/null/%.o: $(ENGINE_DIR)/null/%.c
	@mkdir -p $(dir $@)
	$(GPP) $(BASE_CFLAGS) -o $@ -c $<

$(TARGET): $(ALL_OBJS) linker.ld
	@mkdir -p $(BUILD_DIR)
	$(LD) -melf_i386 -T linker.ld -o $@ $(ALL_OBJS) \
		-L$(LIBHX86_LIB_DIR) -L$(LIBMBEDTLS_LIB_DIR) -lmbedtls -lhx86 \
		--allow-multiple-definition

-include $(ALL_OBJS:.o=.d)

clean:
	rm -rf $(ENGINE_OBJ_DIR)
	rm -f $(TARGET)
