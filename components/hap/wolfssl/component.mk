COMPONENT_SRCDIRS := src wolfcrypt/src
COMPONENT_ADD_INCLUDEDIRS := .
COMPONENT_OBJS := \
    src/internal.o          \
    src/io.o                \
    src/keys.o              \
    src/ocsp.o              \
    src/ssl.o               \
    src/tls.o               \
    wolfcrypt/src/aes.o     \
    wolfcrypt/src/arc4.o    \
    wolfcrypt/src/asn.o     \
    wolfcrypt/src/chacha.o  \
    wolfcrypt/src/chacha20_poly1305.o  \
    wolfcrypt/src/coding.o  \
    wolfcrypt/src/curve25519.o  \
    wolfcrypt/src/dh.o      \
    wolfcrypt/src/ed25519.o \
    wolfcrypt/src/error.o   \
    wolfcrypt/src/fe_operations.o   \
    wolfcrypt/src/ge_operations.o   \
    wolfcrypt/src/hash.o    \
    wolfcrypt/src/hmac.o    \
    wolfcrypt/src/integer.o \
    wolfcrypt/src/logging.o \
    wolfcrypt/src/md5.o     \
    wolfcrypt/src/memory.o  \
    wolfcrypt/src/poly1305.o  \
    wolfcrypt/src/random.o  \
    wolfcrypt/src/rsa.o     \
    wolfcrypt/src/sha.o     \
    wolfcrypt/src/sha256.o  \
    wolfcrypt/src/sha512.o  \
    wolfcrypt/src/srp.o  \
    wolfcrypt/src/wc_port.o \
    wolfcrypt/src/wc_encrypt.o

WOLFSSL_SETTINGS =        \
    -DSIZEOF_LONG_LONG=8  \
    -DSMALL_SESSION_CACHE \
    -DWOLFSSL_SMALL_STACK \
	-DWOLFCRYPT_HAVE_SRP  \
	-DWOLFSSL_SHA512      \
    -DHAVE_CHACHA         \
	-DHAVE_HKDF			  \
    -DHAVE_ONE_TIME_AUTH  \
    -DHAVE_ED25519        \
	-DHAVE_ED25519_KEY_EXPORT\
	-DHAVE_ED25519_KEY_IMPORT\
    -DHAVE_OCSP           \
    -DHAVE_CURVE25519     \
	-DHAVE_POLY1305       \
    -DHAVE_SNI            \
    -DHAVE_TLS_EXTENSIONS \
    -DTIME_OVERRIDES      \
    -DNO_DES              \
    -DNO_DES3             \
    -DNO_DSA              \
    -DNO_ERROR_STRINGS    \
    -DNO_HC128            \
    -DNO_MD4              \
    -DNO_OLD_TLS          \
    -DNO_PSK              \
    -DNO_PWDBASED         \
    -DNO_RC4              \
    -DNO_RABBIT           \
    -DNO_STDIO_FILESYSTEM \
    -DNO_WOLFSSL_DIR      \
    -DNO_DH               \
    -DWOLFSSL_STATIC_RSA  \
    -DWOLFSSL_IAR_ARM     \
    -DNDEBUG              \
	-DWOLFSSL_BASE64_ENCODE \
    -DHAVE_CERTIFICATE_STATUS_REQUEST \
    -DCUSTOM_RAND_GENERATE_SEED=os_get_random

LWIP_INCDIRS = \
    -I$(IDF_PATH)/components/lwip/system \
    -I$(IDF_PATH)/components/lwip/include/lwip \
    -I$(IDF_PATH)/components/lwip/include/lwip/port

FREERTOS_INCDIRS = \
    -I$(IDF_PATH)/components/freertos/include \
    -I$(IDF_PATH)/components/freertos/include/freertos

CFLAGS = \
    -fstrict-volatile-bitfields \
    -ffunction-sections         \
    -fdata-sections             \
    -mlongcalls                 \
    -nostdlib                   \
    -ggdb                       \
    -Os                         \
    -DNDEBUG                    \
    -std=gnu99                  \
    -Wno-old-style-declaration  \
    $(LWIP_INCDIRS)             \
    $(FREERTOS_INCDIRS)         \
    $(WOLFSSL_SETTINGS)
