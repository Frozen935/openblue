#
#  Stack standalone Makefile
#

# Default target is to build the library
all: libbyteblue.a demo

# Allow overriding toolchain from environment
CROSS_COMPILE ?=
CC      := $(CROSS_COMPILE)gcc
AR      := $(CROSS_COMPILE)ar
NM      := $(CROSS_COMPILE)nm
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

# Build flags
# CFLAGS for compiler options, CPPFLAGS for preprocessor options (like -I)
CFLAGS ?= -g -O2 -Wall
CPPFLAGS ?=
# Add flags for dependency generation
CFLAGS += -MMD -MP

# Add flags for GNU source compatibility
CFLAGS += -D_GNU_SOURCE

# Generated config header, must be after .config include
AUTOCONF_H = include/generated/autoconf.h

# Include Kconfig configuration
# This will define CONFIG_... variables
-include .config

# Export them to be available for shell commands in rules
export $(shell sed -e 's/=.*//' -e '/^$$/d' .config)

# Include Directories
CPPFLAGS += -Iinclude
CPPFLAGS += -Iinclude/generated
CPPFLAGS += -Ibluetooth
CPPFLAGS += -Ibluetooth/host
CPPFLAGS += -Ibluetooth/host/classic
CPPFLAGS += -Ibluetooth/audio
CPPFLAGS += -Ibluetooth/mesh
CPPFLAGS += -Ibluetooth/services
CPPFLAGS += -Ibluetooth/lib
CPPFLAGS += -Ibluetooth/services/bas
CPPFLAGS += -Ibluetooth/services/ias
CPPFLAGS += -Ibluetooth/services/nus
CPPFLAGS += -Ibluetooth/services/ots
CPPFLAGS += -Ishim/include
CPPFLAGS += -I.

# Force include the generated config file for all source files
CPPFLAGS += -include $(AUTOCONF_H)

CPPFLAGS += -include shim/include/shim.h

ifneq ($(CONFIG_BYTEBLUE_CRYPTO_USE_MBEDTLS),)
# Prefer system mbed TLS via pkg-config (mbedcrypto or mbedtls). Fallback: local pinned build.
MBEDTLS_PKG_AVAILABLE := $(shell (pkg-config --exists mbedcrypto || pkg-config --exists mbedtls) && echo yes || echo no)

ifeq ($(MBEDTLS_PKG_AVAILABLE),yes)
  MBEDTLS_CFLAGS := $(shell pkg-config --cflags mbedcrypto 2>/dev/null || pkg-config --cflags mbedtls 2>/dev/null)
  MBEDTLS_LIBS   := $(shell pkg-config --libs mbedcrypto 2>/dev/null || pkg-config --libs mbedtls 2>/dev/null)
else
  MBEDTLS_LOCAL_DIR := third_party/mbedtls
  MBEDTLS_LOCAL_TAG := v2.28.8
  MBEDTLS_LOCAL_CRYPTO := $(MBEDTLS_LOCAL_DIR)/library/libmbedcrypto.a
  MBEDTLS_FALLBACK_TARGET := mbedtls-local
  MBEDTLS_CFLAGS := -I$(MBEDTLS_LOCAL_DIR)/include
  MBEDTLS_LIBS   := $(MBEDTLS_LOCAL_CRYPTO)
endif

# Enable PSA via mbed TLS and propagate CFLAGS/INCLUDEs
CFLAGS += $(MBEDTLS_CFLAGS)
endif

# Fallback: local mbed TLS build target (only defined when used)
ifeq ($(MBEDTLS_FALLBACK_TARGET),mbedtls-local)
.PHONY: mbedtls-local
mbedtls-local:
	@echo "Fetching and building local mbed TLS ($(MBEDTLS_LOCAL_TAG))..."
	@test -d $(MBEDTLS_LOCAL_DIR) || git clone --depth 1 --branch $(MBEDTLS_LOCAL_TAG) https://github.com/Mbed-TLS/mbedtls.git $(MBEDTLS_LOCAL_DIR)
	@$(MAKE) -C $(MBEDTLS_LOCAL_DIR)/library libmbedcrypto.a
endif

# Source files
CSRCS :=
CSRCS += osdep/posix/os.c

# Core init registry
CSRCS += core/stack_init.c

CSRCS_BASE :=
CSRCS_BASE += base/bt_atomic.c
CSRCS_BASE += base/bt_buf.c
CSRCS_BASE += base/bt_buf_simple.c
CSRCS_BASE += base/utils.c
CSRCS_BASE += base/queue/bt_queue.c
CSRCS_BASE += base/bt_mem_pool.c
CSRCS_BASE += base/bt_poll.c
CSRCS_BASE += base/bt_work.c
CSRCS_BASE += base/log.c

CSRCS_DRIVERS :=
CSRCS_DRIVERS += drivers/hci_sock.c
CSRCS_DRIVERS += drivers/userchan.c

# Common sources
CSRCS_COMMON = \
    bluetooth/common/addr.c \
    bluetooth/common/dummy.c \
    bluetooth/common/bt_str.c

ifeq ($(CONFIG_BT_RPA),y)
  CSRCS_COMMON += bluetooth/common/rpa.c
endif
ifeq ($(CONFIG_BT_PRIVATE_SHELL),y)
  CSRCS_COMMON += bluetooth/common/bt_shell_private.c
endif

# Crypto sources
CSRCS_CRYPTO =
ifeq ($(CONFIG_BT_CRYPTO),y)
  CSRCS_CRYPTO += \
      bluetooth/crypto/bt_crypto.c \
      bluetooth/crypto/bt_crypto_psa.c
endif

# Host sources
CSRCS_HOST =
ifeq ($(CONFIG_BT_HCI_HOST),y)
  CSRCS_HOST += \
      bluetooth/host/uuid.c \
      bluetooth/host/addr.c \
      bluetooth/host/buf.c \
      bluetooth/host/data.c \
      bluetooth/host/hci_core.c \
      bluetooth/host/hci_common.c \
      bluetooth/host/id.c

  ifeq ($(CONFIG_BT_BROADCASTER),y)
    CSRCS_HOST += bluetooth/host/adv.c
  endif
  ifeq ($(CONFIG_BT_OBSERVER),y)
    CSRCS_HOST += bluetooth/host/scan.c
  endif
  ifeq ($(CONFIG_BT_HOST_CRYPTO),y)
    CSRCS_HOST += bluetooth/host/crypto_psa.c
  endif
  ifeq ($(CONFIG_BT_ECC),y)
    CSRCS_HOST += bluetooth/host/ecc.c
  endif

  ifeq ($(CONFIG_BT_CONN),y)
    CSRCS_HOST += \
        bluetooth/host/conn.c \
        bluetooth/host/l2cap.c \
        bluetooth/host/att.c \
        bluetooth/host/gatt.c

    ifeq ($(CONFIG_BT_SMP),y)
      CSRCS_HOST += \
          bluetooth/host/smp.c \
          bluetooth/host/keys.c
    else
      CSRCS_HOST += bluetooth/host/smp_null.c
    endif
  endif

  ifeq ($(CONFIG_BT_ISO),y)
    CSRCS_HOST += bluetooth/host/iso.c
    # conn.c is already added with CONFIG_BT_CONN, but also needed for ISO
    ifneq ($(CONFIG_BT_CONN),y)
      CSRCS_HOST += bluetooth/host/conn.c
    endif
  endif

  ifeq ($(CONFIG_BT_CHANNEL_SOUNDING),y)
    CSRCS_HOST += bluetooth/host/cs.c
  endif
  ifeq ($(CONFIG_BT_DF),y)
    CSRCS_HOST += bluetooth/host/direction.c
  endif
  ifeq ($(CONFIG_BT_HCI_RAW),y)
    CSRCS_HOST += bluetooth/host/hci_raw.c
  endif
  ifeq ($(CONFIG_BT_MONITOR),y)
    CSRCS_HOST += bluetooth/host/monitor.c
  endif
  ifeq ($(CONFIG_BT_SETTINGS),y)
    CSRCS_HOST += bluetooth/host/settings.c
  endif
  ifeq ($(CONFIG_BT_HOST_CCM),y)
    CSRCS_HOST += bluetooth/host/aes_ccm.c
  endif
  ifeq ($(CONFIG_BT_LONG_WQ),y)
    CSRCS_HOST += bluetooth/host/long_wq.c
  endif
endif

# Host Classic sources
CSRCS_HOST_CLASSIC =
ifeq ($(CONFIG_BT_CLASSIC),y)
  CSRCS_HOST_CLASSIC += \
      bluetooth/host/classic/br.c \
      bluetooth/host/classic/conn_br.c \
      bluetooth/host/classic/keys_br.c \
      bluetooth/host/classic/l2cap_br.c \
      bluetooth/host/classic/sdp.c \
      bluetooth/host/classic/ssp.c \
      bluetooth/host/classic/sco.c

  ifeq ($(CONFIG_BT_A2DP),y)
    CSRCS_HOST_CLASSIC += bluetooth/host/classic/a2dp.c bluetooth/host/classic/a2dp_codec_sbc.c
  endif
  ifeq ($(CONFIG_BT_AVDTP),y)
    CSRCS_HOST_CLASSIC += bluetooth/host/classic/avdtp.c
  endif
  ifeq ($(CONFIG_BT_AVRCP),y)
    CSRCS_HOST_CLASSIC += bluetooth/host/classic/avrcp.c
  endif
  ifeq ($(CONFIG_BT_AVCTP),y)
    CSRCS_HOST_CLASSIC += bluetooth/host/classic/avctp.c
  endif
  ifeq ($(CONFIG_BT_RFCOMM),y)
    CSRCS_HOST_CLASSIC += bluetooth/host/classic/rfcomm.c
  endif
  ifeq ($(CONFIG_BT_HFP_HF),y)
    CSRCS_HOST_CLASSIC += bluetooth/host/classic/hfp_hf.c bluetooth/host/classic/at.c
  endif
  ifeq ($(CONFIG_BT_HFP_AG),y)
    CSRCS_HOST_CLASSIC += bluetooth/host/classic/hfp_ag.c
  endif
  ifeq ($(CONFIG_BT_GOEP),y)
    CSRCS_HOST_CLASSIC += bluetooth/host/classic/obex.c bluetooth/host/classic/goep.c
  endif
  ifeq ($(CONFIG_BT_DID),y)
    CSRCS_HOST_CLASSIC += bluetooth/host/classic/did.c
  endif
endif

# Host Shell sources
CSRCS_HOST_SHELL =
ifeq ($(CONFIG_BT_SHELL),y)
  CSRCS_HOST_SHELL += bluetooth/host/shell/bt.c
  ifeq ($(CONFIG_BT_CONN),y)
    CSRCS_HOST_SHELL += bluetooth/host/shell/gatt.c
  endif
  ifeq ($(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL),y)
    CSRCS_HOST_SHELL += bluetooth/host/shell/l2cap.c
  endif
  ifeq ($(CONFIG_BT_ISO),y)
    CSRCS_HOST_SHELL += bluetooth/host/shell/iso.c
  endif
  ifeq ($(CONFIG_BT_CHANNEL_SOUNDING),y)
    CSRCS_HOST_SHELL += bluetooth/host/shell/cs.c
  endif

  # Host Classic Shell sources
  ifeq ($(CONFIG_BT_CLASSIC),y)
    CSRCS_HOST_SHELL += bluetooth/host/classic/shell/bredr.c
    ifeq ($(CONFIG_BT_RFCOMM),y)
      CSRCS_HOST_SHELL += bluetooth/host/classic/shell/rfcomm.c
    endif
    ifeq ($(CONFIG_BT_A2DP),y)
      CSRCS_HOST_SHELL += bluetooth/host/classic/shell/a2dp.c
    endif
    ifeq ($(CONFIG_BT_AVRCP),y)
      CSRCS_HOST_SHELL += bluetooth/host/classic/shell/avrcp.c
    endif
    ifneq ($(filter y,$(CONFIG_BT_HFP_HF) $(CONFIG_BT_HFP_AG)),)
      CSRCS_HOST_SHELL += bluetooth/host/classic/shell/hfp.c
    endif
    ifeq ($(CONFIG_BT_GOEP),y)
      CSRCS_HOST_SHELL += bluetooth/host/classic/shell/goep.c
    endif
  endif
endif

# Audio sources
CSRCS_AUDIO =
ifeq ($(CONFIG_BT_AUDIO),y)
  CSRCS_AUDIO += bluetooth/audio/audio.c

  ifneq ($(filter y,$(CONFIG_BT_VOCS) $(CONFIG_BT_VOCS_CLIENT)),)
    CSRCS_AUDIO += bluetooth/audio/vocs.c
  endif
  ifeq ($(CONFIG_BT_VOCS_CLIENT),y)
    CSRCS_AUDIO += bluetooth/audio/vocs_client.c
  endif

  ifneq ($(filter y,$(CONFIG_BT_AICS) $(CONFIG_BT_AICS_CLIENT)),)
    CSRCS_AUDIO += bluetooth/audio/aics.c
  endif
  ifeq ($(CONFIG_BT_AICS_CLIENT),y)
    CSRCS_AUDIO += bluetooth/audio/aics_client.c
  endif

  ifeq ($(CONFIG_BT_VCP_VOL_REND),y)
    CSRCS_AUDIO += bluetooth/audio/vcp_vol_rend.c
  endif
  ifeq ($(CONFIG_BT_VCP_VOL_CTLR),y)
    CSRCS_AUDIO += bluetooth/audio/vcp_vol_ctlr.c
  endif

  ifeq ($(CONFIG_BT_MICP_MIC_DEV),y)
    CSRCS_AUDIO += bluetooth/audio/micp_mic_dev.c
  endif
  ifeq ($(CONFIG_BT_MICP_MIC_CTLR),y)
    CSRCS_AUDIO += bluetooth/audio/micp_mic_ctlr.c
  endif

  ifeq ($(CONFIG_BT_CONN),y)
    CSRCS_AUDIO += bluetooth/audio/ccid.c
  endif

  ifeq ($(CONFIG_BT_CSIP_SET_MEMBER),y)
    CSRCS_AUDIO += bluetooth/audio/csip_set_member.c
  endif
  ifeq ($(CONFIG_BT_CSIP_SET_COORDINATOR),y)
    CSRCS_AUDIO += bluetooth/audio/csip_set_coordinator.c
  endif
  ifneq ($(filter y,$(CONFIG_BT_CSIP_SET_MEMBER) $(CONFIG_BT_CSIP_SET_COORDINATOR)),)
    CSRCS_AUDIO += bluetooth/audio/csip_crypto.c
  endif

  ifeq ($(CONFIG_BT_TBS),y)
    CSRCS_AUDIO += bluetooth/audio/tbs.c
  endif
  ifeq ($(CONFIG_BT_TBS_CLIENT),y)
    CSRCS_AUDIO += bluetooth/audio/tbs_client.c
  endif
  ifeq ($(CONFIG_BT_MCC),y)
    CSRCS_AUDIO += bluetooth/audio/mcc.c
  endif
  ifeq ($(CONFIG_BT_MCS),y)
    CSRCS_AUDIO += bluetooth/audio/mcs.c
  endif
  ifeq ($(CONFIG_BT_MPL),y)
    CSRCS_AUDIO += bluetooth/audio/mpl.c
  endif
  ifeq ($(CONFIG_MCTL),y)
    CSRCS_AUDIO += bluetooth/audio/media_proxy.c
  endif
  ifeq ($(CONFIG_BT_ASCS),y)
    CSRCS_AUDIO += bluetooth/audio/ascs.c
  endif
  ifeq ($(CONFIG_BT_PACS),y)
    CSRCS_AUDIO += bluetooth/audio/pacs.c
  endif
  ifeq ($(CONFIG_BT_BAP_STREAM),y)
    CSRCS_AUDIO += bluetooth/audio/bap_stream.c bluetooth/audio/codec.c bluetooth/audio/bap_iso.c
  endif
  ifeq ($(CONFIG_BT_BAP_BASE),y)
    CSRCS_AUDIO += bluetooth/audio/bap_base.c
  endif
  ifeq ($(CONFIG_BT_BAP_UNICAST_SERVER),y)
    CSRCS_AUDIO += bluetooth/audio/bap_unicast_server.c
  endif
  ifeq ($(CONFIG_BT_BAP_UNICAST_CLIENT),y)
    CSRCS_AUDIO += bluetooth/audio/bap_unicast_client.c
  endif
  ifeq ($(CONFIG_BT_BAP_BROADCAST_SOURCE),y)
    CSRCS_AUDIO += bluetooth/audio/bap_broadcast_source.c
  endif
  ifeq ($(CONFIG_BT_BAP_BROADCAST_SINK),y)
    CSRCS_AUDIO += bluetooth/audio/bap_broadcast_sink.c
  endif
  ifeq ($(CONFIG_BT_BAP_SCAN_DELEGATOR),y)
    CSRCS_AUDIO += bluetooth/audio/bap_scan_delegator.c
  endif
  ifeq ($(CONFIG_BT_BAP_BROADCAST_ASSISTANT),y)
    CSRCS_AUDIO += bluetooth/audio/bap_broadcast_assistant.c
  endif
  ifeq ($(CONFIG_BT_CCP_CALL_CONTROL_CLIENT),y)
    CSRCS_AUDIO += bluetooth/audio/ccp_call_control_client.c
  endif
  ifeq ($(CONFIG_BT_CCP_CALL_CONTROL_SERVER),y)
    CSRCS_AUDIO += bluetooth/audio/ccp_call_control_server.c
  endif
  ifeq ($(CONFIG_BT_HAS),y)
    CSRCS_AUDIO += bluetooth/audio/has.c
  endif
  ifeq ($(CONFIG_BT_HAS_CLIENT),y)
    CSRCS_AUDIO += bluetooth/audio/has_client.c
  endif
  ifeq ($(CONFIG_BT_CAP),y)
    CSRCS_AUDIO += bluetooth/audio/cap_stream.c
  endif
  ifeq ($(CONFIG_BT_CAP_ACCEPTOR),y)
    CSRCS_AUDIO += bluetooth/audio/cap_acceptor.c
  endif
  ifeq ($(CONFIG_BT_CAP_INITIATOR),y)
    CSRCS_AUDIO += bluetooth/audio/cap_initiator.c
  endif
  ifeq ($(CONFIG_BT_CAP_COMMANDER),y)
    CSRCS_AUDIO += bluetooth/audio/cap_commander.c
  endif
  ifeq ($(CONFIG_BT_CAP_HANDOVER),y)
    CSRCS_AUDIO += bluetooth/audio/cap_handover.c
  endif
  ifneq ($(filter y,$(CONFIG_BT_CAP_INITIATOR_UNICAST) $(CONFIG_BT_CAP_COMMANDER)),)
    CSRCS_AUDIO += bluetooth/audio/cap_common.c
  endif
  ifeq ($(CONFIG_BT_TMAP),y)
    CSRCS_AUDIO += bluetooth/audio/tmap.c
  endif
  ifeq ($(CONFIG_BT_GMAP),y)
    CSRCS_AUDIO += bluetooth/audio/gmap_client.c bluetooth/audio/gmap_server.c
  endif
  ifeq ($(CONFIG_BT_PBP),y)
    CSRCS_AUDIO += bluetooth/audio/pbp.c
  endif
endif

# Audio Shell sources
CSRCS_AUDIO_SHELL =
ifeq ($(CONFIG_BT_SHELL),y)
  ifeq ($(CONFIG_BT_CCP_CALL_CONTROL_SERVER),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/ccp_call_control_server.c
  endif
  ifeq ($(CONFIG_BT_CCP_CALL_CONTROL_CLIENT),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/ccp_call_control_client.c
  endif
  ifeq ($(CONFIG_BT_VCP_VOL_REND),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/vcp_vol_rend.c
  endif
  ifeq ($(CONFIG_BT_VCP_VOL_CTLR),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/vcp_vol_ctlr.c
  endif
  ifeq ($(CONFIG_BT_MICP_MIC_DEV),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/micp_mic_dev.c
  endif
  ifeq ($(CONFIG_BT_MICP_MIC_CTLR),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/micp_mic_ctlr.c
  endif
  ifeq ($(CONFIG_BT_CSIP_SET_MEMBER),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/csip_set_member.c
  endif
  ifeq ($(CONFIG_BT_CSIP_SET_COORDINATOR),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/csip_set_coordinator.c
  endif
  ifeq ($(CONFIG_BT_TBS),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/tbs.c
  endif
  ifeq ($(CONFIG_BT_TBS_CLIENT),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/tbs_client.c
  endif
  ifeq ($(CONFIG_BT_MPL),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/mpl.c
  endif
  ifeq ($(CONFIG_BT_MCC),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/mcc.c
  endif
  ifeq ($(CONFIG_BT_MCS),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/media_controller.c
  endif
  ifeq ($(CONFIG_BT_HAS_PRESET_SUPPORT),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/has.c
  endif
  ifeq ($(CONFIG_BT_CAP_ACCEPTOR),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/cap_acceptor.c
  endif
  ifeq ($(CONFIG_BT_CAP_INITIATOR),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/cap_initiator.c
  endif
  ifeq ($(CONFIG_BT_CAP_COMMANDER),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/cap_commander.c
  endif
  ifeq ($(CONFIG_BT_HAS_CLIENT),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/has_client.c
  endif
  ifeq ($(CONFIG_BT_TMAP),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/tmap.c
  endif
  ifeq ($(CONFIG_BT_GMAP),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/gmap.c
  endif
  ifeq ($(CONFIG_BT_BAP_STREAM),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/bap.c
  endif
  ifeq ($(CONFIG_LIBLC3),y)
    ifeq ($(CONFIG_USBD_AUDIO2_CLASS),y)
      CSRCS_AUDIO_SHELL += bluetooth/audio/shell/bap_usb.c
    endif
  endif
  ifeq ($(CONFIG_BT_BAP_SCAN_DELEGATOR),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/bap_scan_delegator.c
  endif
  ifeq ($(CONFIG_BT_BAP_BROADCAST_ASSISTANT),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/bap_broadcast_assistant.c
  endif
  ifeq ($(CONFIG_BT_PBP),y)
    CSRCS_AUDIO_SHELL += bluetooth/audio/shell/pbp.c
  endif
endif

# Services sources
CSRCS_SERVICES =
ifeq ($(CONFIG_BT_CONN),y)
  ifeq ($(CONFIG_BT_DIS),y)
    CSRCS_SERVICES += bluetooth/services/dis.c
  endif
  ifeq ($(CONFIG_BT_CTS),y)
    CSRCS_SERVICES += bluetooth/services/cts.c
  endif
  ifeq ($(CONFIG_BT_HRS),y)
    CSRCS_SERVICES += bluetooth/services/hrs.c
  endif
  ifeq ($(CONFIG_BT_TPS),y)
    CSRCS_SERVICES += bluetooth/services/tps.c
  endif
  ifeq ($(CONFIG_BT_BAS),y)
    # Assuming bas sources are in a subdirectory, handled by separate logic if complex
  endif
  ifneq ($(filter y,$(CONFIG_BT_OTS) $(CONFIG_BT_OTS_CLIENT)),)
    # Assuming ots sources are in a subdirectory
  endif
  ifneq ($(filter y,$(CONFIG_BT_IAS) $(CONFIG_BT_IAS_CLIENT)),)
    # Assuming ias sources are in a subdirectory
  endif
endif

# Mesh sources
CSRCS_MESH =
ifeq ($(CONFIG_BT_MESH),y)
  CSRCS_MESH += \
      bluetooth/mesh/main.c \
      bluetooth/mesh/cfg.c \
      bluetooth/mesh/adv.c \
      bluetooth/mesh/beacon.c \
      bluetooth/mesh/net.c \
      bluetooth/mesh/subnet.c \
      bluetooth/mesh/app_keys.c \
      bluetooth/mesh/heartbeat.c \
      bluetooth/mesh/crypto.c \
      bluetooth/mesh/crypto_psa.c \
      bluetooth/mesh/access.c \
      bluetooth/mesh/msg.c \
      bluetooth/mesh/cfg_srv.c \
      bluetooth/mesh/health_srv.c \
      bluetooth/mesh/va.c \
      bluetooth/mesh/transport.c

  ifeq ($(CONFIG_BT_MESH_ADV_LEGACY),y)
    CSRCS_MESH += bluetooth/mesh/adv_legacy.c
  endif
  ifeq ($(CONFIG_BT_MESH_ADV_EXT),y)
    CSRCS_MESH += bluetooth/mesh/adv_ext.c
  endif
  ifeq ($(CONFIG_BT_SETTINGS),y)
    CSRCS_MESH += bluetooth/mesh/settings.c
  endif
  ifeq ($(CONFIG_BT_MESH_RPL_STORAGE_MODE_SETTINGS),y)
    CSRCS_MESH += bluetooth/mesh/rpl.c
  endif
  ifeq ($(CONFIG_BT_MESH_LOW_POWER),y)
    CSRCS_MESH += bluetooth/mesh/lpn.c
  endif
  ifeq ($(CONFIG_BT_MESH_FRIEND),y)
    CSRCS_MESH += bluetooth/mesh/friend.c
  endif
  ifeq ($(CONFIG_BT_MESH_PROV),y)
    CSRCS_MESH += bluetooth/mesh/prov.c
  endif
  ifeq ($(CONFIG_BT_MESH_PROVISIONEE),y)
    CSRCS_MESH += bluetooth/mesh/provisionee.c
  endif
  ifeq ($(CONFIG_BT_MESH_PROVISIONER),y)
    CSRCS_MESH += bluetooth/mesh/provisioner.c
  endif
  ifeq ($(CONFIG_BT_MESH_PB_ADV),y)
    CSRCS_MESH += bluetooth/mesh/pb_adv.c
  endif
  ifeq ($(CONFIG_BT_MESH_PB_GATT_COMMON),y)
    CSRCS_MESH += bluetooth/mesh/pb_gatt.c
  endif
  ifeq ($(CONFIG_BT_MESH_PB_GATT),y)
    CSRCS_MESH += bluetooth/mesh/pb_gatt_srv.c
  endif
  ifeq ($(CONFIG_BT_MESH_PB_GATT_CLIENT),y)
    CSRCS_MESH += bluetooth/mesh/pb_gatt_cli.c
  endif
  ifeq ($(CONFIG_BT_MESH_GATT_CLIENT),y)
    CSRCS_MESH += bluetooth/mesh/gatt_cli.c
  endif
  ifeq ($(CONFIG_BT_MESH_PROXY_CLIENT),y)
    CSRCS_MESH += bluetooth/mesh/proxy_cli.c
  endif
  ifeq ($(CONFIG_BT_MESH_GATT_PROXY),y)
    CSRCS_MESH += bluetooth/mesh/proxy_srv.c
  endif
  ifeq ($(CONFIG_BT_MESH_GATT),y)
    CSRCS_MESH += bluetooth/mesh/proxy_msg.c
  endif
  ifeq ($(CONFIG_BT_MESH_CFG_CLI),y)
    CSRCS_MESH += bluetooth/mesh/cfg_cli.c
  endif
  ifeq ($(CONFIG_BT_MESH_HEALTH_CLI),y)
    CSRCS_MESH += bluetooth/mesh/health_cli.c
  endif
  ifeq ($(CONFIG_BT_MESH_SAR_CFG_CLI),y)
    CSRCS_MESH += bluetooth/mesh/sar_cfg_cli.c
  endif
  ifeq ($(CONFIG_BT_MESH_SAR_CFG_SRV),y)
    CSRCS_MESH += bluetooth/mesh/sar_cfg_srv.c
  endif
  ifeq ($(CONFIG_BT_MESH_SAR_CFG),y)
    CSRCS_MESH += bluetooth/mesh/sar_cfg.c
  endif
  ifeq ($(CONFIG_BT_MESH_OP_AGG_SRV),y)
    CSRCS_MESH += bluetooth/mesh/op_agg_srv.c
  endif
  ifeq ($(CONFIG_BT_MESH_OP_AGG_CLI),y)
    CSRCS_MESH += bluetooth/mesh/op_agg_cli.c
  endif
  ifeq ($(CONFIG_BT_MESH_OP_AGG),y)
    CSRCS_MESH += bluetooth/mesh/op_agg.c
  endif
  ifeq ($(CONFIG_BT_MESH_LARGE_COMP_DATA_SRV),y)
    CSRCS_MESH += bluetooth/mesh/large_comp_data_srv.c
  endif
  ifeq ($(CONFIG_BT_MESH_LARGE_COMP_DATA_CLI),y)
    CSRCS_MESH += bluetooth/mesh/large_comp_data_cli.c
  endif
  ifeq ($(CONFIG_BT_MESH_PRIV_BEACON_SRV),y)
    CSRCS_MESH += bluetooth/mesh/priv_beacon_srv.c
  endif
  ifeq ($(CONFIG_BT_MESH_PRIV_BEACON_CLI),y)
    CSRCS_MESH += bluetooth/mesh/priv_beacon_cli.c
  endif
  ifeq ($(CONFIG_BT_MESH_SELF_TEST),y)
    CSRCS_MESH += bluetooth/mesh/test.c
  endif
  ifeq ($(CONFIG_BT_MESH_CDB),y)
    CSRCS_MESH += bluetooth/mesh/cdb.c
  endif
  ifeq ($(CONFIG_BT_MESH_BLOB_SRV),y)
    CSRCS_MESH += bluetooth/mesh/blob_srv.c
  endif
  ifeq ($(CONFIG_BT_MESH_BLOB_CLI),y)
    CSRCS_MESH += bluetooth/mesh/blob_cli.c
  endif
  ifeq ($(CONFIG_BT_MESH_BLOB_IO_FLASH),y)
    CSRCS_MESH += bluetooth/mesh/blob_io_flash.c
  endif
  ifeq ($(CONFIG_BT_MESH_DFU_CLI),y)
    CSRCS_MESH += bluetooth/mesh/dfu_cli.c
  endif
  ifeq ($(CONFIG_BT_MESH_DFU_SRV),y)
    CSRCS_MESH += bluetooth/mesh/dfu_srv.c
  endif
  ifeq ($(CONFIG_BT_MESH_DFD_SRV),y)
    CSRCS_MESH += bluetooth/mesh/dfd_srv.c
  endif
  ifeq ($(CONFIG_BT_MESH_DFU_SLOTS),y)
    CSRCS_MESH += bluetooth/mesh/dfu_slot.c
  endif
  ifeq ($(CONFIG_BT_MESH_DFU_METADATA),y)
    CSRCS_MESH += bluetooth/mesh/dfu_metadata.c
  endif
  ifeq ($(CONFIG_BT_MESH_RPR_CLI),y)
    CSRCS_MESH += bluetooth/mesh/rpr_cli.c
  endif
  ifeq ($(CONFIG_BT_MESH_RPR_SRV),y)
    CSRCS_MESH += bluetooth/mesh/rpr_srv.c
  endif
  ifeq ($(CONFIG_BT_MESH_OD_PRIV_PROXY_CLI),y)
    CSRCS_MESH += bluetooth/mesh/od_priv_proxy_cli.c
  endif
  ifeq ($(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV),y)
    CSRCS_MESH += bluetooth/mesh/od_priv_proxy_srv.c
  endif
  ifeq ($(CONFIG_BT_MESH_SOL_PDU_RPL_CLI),y)
    CSRCS_MESH += bluetooth/mesh/sol_pdu_rpl_cli.c
  endif
  ifeq ($(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV),y)
    CSRCS_MESH += bluetooth/mesh/sol_pdu_rpl_srv.c
  endif
  ifeq ($(CONFIG_BT_MESH_BRG_CFG_CLI),y)
    CSRCS_MESH += bluetooth/mesh/brg_cfg_cli.c
  endif
  ifeq ($(CONFIG_BT_MESH_BRG_CFG_SRV),y)
    CSRCS_MESH += bluetooth/mesh/brg_cfg_srv.c bluetooth/mesh/brg_cfg.c
  endif
  ifeq ($(CONFIG_BT_MESH_SOLICITATION),y)
    CSRCS_MESH += bluetooth/mesh/solicitation.c
  endif
  ifeq ($(CONFIG_BT_MESH_STATISTIC),y)
    CSRCS_MESH += bluetooth/mesh/statistic.c
  endif
  ifeq ($(CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG),y)
    CSRCS_MESH += bluetooth/mesh/delayable_msg.c
  endif
  ifeq ($(CONFIG_BT_TESTING),y)
    CSRCS_MESH += bluetooth/mesh/testing.c
  endif
endif

# Lib sources
CSRCS_LIB =
ifeq ($(CONFIG_BT_EAD),y)
  CSRCS_LIB += bluetooth/lib/ead.c
endif

# Combine all source files
CSRCS += \
    $(CSRCS_BASE) \
    $(CSRCS_DRIVERS) \
    $(CSRCS_COMMON) \
    $(CSRCS_CRYPTO) \
    $(CSRCS_HOST) \
    $(CSRCS_HOST_CLASSIC) \
    $(CSRCS_HOST_SHELL) \
    $(CSRCS_AUDIO) \
    $(CSRCS_AUDIO_SHELL) \
    $(CSRCS_SERVICES) \
    $(CSRCS_MESH) \
    $(CSRCS_LIB)

# Object files
OBJS = $(CSRCS:.c=.o)

# Generated dependency files
DEPS = $(OBJS:.o=.d)

# Ensure local mbed TLS headers/libs are ready before compiling objects
ifeq ($(MBEDTLS_FALLBACK_TARGET),mbedtls-local)
$(OBJS): | mbedtls-local
endif

# Make all objects depend on the generated config header
$(OBJS): $(AUTOCONF_H)

# Rule to generate the config header
$(AUTOCONF_H): .config kconfig-deps
	@echo "Generating config header $< -> $@"
	@mkdir -p $(dir $@)
	@python3 tools/kconfig/genconfig.py --header-output $@ .config

# Main build target
libbyteblue.a: $(OBJS)
	@echo "AR $@"
	$(AR) rcs $@ $(OBJS)

# Rule to compile a .c file to a .o file
%.o: %.c
	@echo "CC $<"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Include generated dependency files
-include $(DEPS)

# Clean target
clean:
	@echo "Cleaning..."
	$(RM) $(OBJS) $(DEPS) libbyteblue.a samples/demo/demo samples/demo/demo.d
	$(RM) -r include/generated

# Samples targets
.PHONY: samples demo

samples: demo

demo: $(MBEDTLS_FALLBACK_TARGET) libbyteblue.a samples/demo/main.c
	@echo "Building samples/demo..."
	$(CC) $(CFLAGS) $(CPPFLAGS) -o samples/demo/demo samples/demo/main.c libbyteblue.a $(MBEDTLS_LIBS) -lpthread -lrt

# Kconfig targets
.PHONY: menuconfig kconfig-deps help genconfig

help:
	@echo "Targets:"
	@echo "  make              - build libbyteblue.a"
	@echo "  make menuconfig   - run Kconfig menuconfig UI and write .config"
	@echo "  make genconfig    - force generate header file from .config"
	@echo "  make clean        - remove built files"
	@echo "  make demo         - build samples/demo"
	@echo "  make samples      - build all samples (currently demo)"

kconfig-deps:
	@python3 -c "import kconfiglib" 2>/dev/null || pip3 install --user kconfiglib

menuconfig: kconfig-deps
	@python3 tools/kconfig/run_menuconfig.py Kconfig .config