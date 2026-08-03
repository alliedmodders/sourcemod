/* lib/curl_config.h.  Generated from curl_config.h.in by configure.  */
/* lib/curl_config.h.in.  Generated from configure.ac by autoheader.  */

/* to enable curl debug memory tracking */
/* #undef CURLDEBUG */

/* Location of default ca bundle */
/* #undef CURL_CA_BUNDLE */

/* define "1" to use built in CA store of SSL library */
/* #undef CURL_CA_FALLBACK */

/* Location of default ca path */
/* #undef CURL_CA_PATH */

/* Default SSL backend */
/* #undef CURL_DEFAULT_SSL_BACKEND */

/* disable alt-svc */
#define CURL_DISABLE_ALTSVC 1

/* to disable cookies support */
#define CURL_DISABLE_COOKIES 1

/* to disable cryptographic authentication */
#define CURL_DISABLE_CRYPTO_AUTH 1

/* to disable DICT */
#define CURL_DISABLE_DICT 1

/* disable DoH */
/* #undef CURL_DISABLE_DOH */

/* to disable FILE */
#define CURL_DISABLE_FILE 1

/* to disable FTP */
#define CURL_DISABLE_FTP 1

/* to disable curl_easy_options */
/* #undef CURL_DISABLE_GETOPTIONS */

/* to disable Gopher */
#define CURL_DISABLE_GOPHER 1

/* disable headers-api */
/* #undef CURL_DISABLE_HEADERS_API */

/* disable alt-svc */
/* #undef CURL_DISABLE_HSTS */

/* to disable HTTP */
/* #undef CURL_DISABLE_HTTP */

/* disable HTTP authentication */
/* #undef CURL_DISABLE_HTTP_AUTH */

/* to disable IMAP */
#define CURL_DISABLE_IMAP 1

/* to disable LDAP */
#define CURL_DISABLE_LDAP 1

/* to disable LDAPS */
#define CURL_DISABLE_LDAPS 1

/* to disable --libcurl C code generation option */
/* #undef CURL_DISABLE_LIBCURL_OPTION */

/* disable mime API */
/* #undef CURL_DISABLE_MIME */

/* to disable MQTT */
#define CURL_DISABLE_MQTT 1

/* disable netrc parsing */
#define CURL_DISABLE_NETRC 1

/* to disable NTLM support */
#define CURL_DISABLE_NTLM 1

/* if the OpenSSL configuration won't be loaded automatically */
/* #undef CURL_DISABLE_OPENSSL_AUTO_LOAD_CONFIG */

/* disable date parsing */
/* #undef CURL_DISABLE_PARSEDATE */

/* to disable POP3 */
#define CURL_DISABLE_POP3 1

/* disable progress-meter */
/* #undef CURL_DISABLE_PROGRESS_METER */

/* to disable proxies */
#define CURL_DISABLE_PROXY 1

/* to disable RTSP */
#define CURL_DISABLE_RTSP 1

/* disable DNS shuffling */
#define CURL_DISABLE_SHUFFLE_DNS 1

/* to disable SMB/CIFS */
#define CURL_DISABLE_SMB 1

/* to disable SMTP */
#define CURL_DISABLE_SMTP 1

/* to disable socketpair support */
#define CURL_DISABLE_SOCKETPAIR 1

/* to disable TELNET */
#define CURL_DISABLE_TELNET 1

/* to disable TFTP */
#define CURL_DISABLE_TFTP 1

/* if mbedTLS is enabled */
#define USE_MBEDTLS 1