#include <libwebsockets.h>
#include <string.h>
#include <signal.h>

extern const lws_ss_info_t ssi_client, ssi_server;

static struct lws_context *context;
int interrupted, tests_bad = 1, multipart;
static const char * const default_ss_policy =
        "{"
          "\"release\":"			"\"01234567\","
          "\"product\":"			"\"myproduct\","
          "\"schema-version\":"			"1,"
          "\"retry\": ["	/* named backoff / retry strategies */
                "{\"default\": {"
                        "\"backoff\": ["	 "1000,"
                                                 "2000,"
                                                 "3000,"
                                                 "5000,"
                                                "10000"
                                "],"
                        "\"conceal\":"		"5,"
                        "\"jitterpc\":"		"20,"
                        "\"svalidping\":"	"300,"
                        "\"svalidhup\":"	"310"
                "}}"
          "],"
          "\"certs\": [" /* named individual certificates in BASE64 DER */
                /*
                 * Need to be in order from root cert... notice sometimes as
                 * with Let's Encrypt there are multiple possible validation
                 * paths, all the pieces for one validation path must be
                 * given, excluding the server cert itself.  Let's Encrypt
                 * intermediate is signed by their ISRG Root CA but also is
                 * cross-signed by an IdenTrust intermediate that's widely
                 * deployed in browsers.  We use the ISRG path because that
                 * way we can skip the extra IdenTrust root cert.
                 */
                        "{\"isrg_root_x1\": \""
        "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"
          "\"},"
                /*
                 * a selfsigned cert for localhost for 100 years
                 */
                "{\"self_localhost\": \""
"MIIHNjCCBR6gAwIBAgIUd6Ml1sauT8PwYdQ/7qTz6M967F4wDQYJKoZIhvcNAQEN"
"BQAwgaExCzAJBgNVBAYTAlNHMRIwEAYDVQQIDAlTaW5nYXBvcmUxEjAQBgNVBAcM"
"CVNpbmdhcG9yZTEWMBQGA1UECgwNVmlydHVhbCBTa2llczELMAkGA1UECwwCSVQx"
"IDAeBgNVBAMMF3N2ci52aXJ0dWFsc2tpZXMuY29tLnNnMSMwIQYJKoZIhvcNAQkB"
"FhRqYWNreTgxMTAwQHlhaG9vLmNvbTAeFw0yNTA3MDEwMTMxNDlaFw0zNTA2Mjkw"
"MTMxNDlaMIGhMQswCQYDVQQGEwJTRzESMBAGA1UECAwJU2luZ2Fwb3JlMRIwEAYD"
"VQQHDAlTaW5nYXBvcmUxFjAUBgNVBAoMDVZpcnR1YWwgU2tpZXMxCzAJBgNVBAsM"
"AklUMSAwHgYDVQQDDBdzdnIudmlydHVhbHNraWVzLmNvbS5zZzEjMCEGCSqGSIb3"
"DQEJARYUamFja3k4MTEwMEB5YWhvby5jb20wggIiMA0GCSqGSIb3DQEBAQUAA4IC"
"DwAwggIKAoICAQDNSzjMhmdyFHFY4JgTU9XTaJPOMP9fPtsZ5pfonqltk71fu+dN"
"drWRL8zMyaJ8pvA2re/cLYM14PWabEKvmfLvwLWH0g4a7F945f6fOV+eX3PeVbHr"
"h2pgdSrEspp9FVO8f4HQpHAt82+V3Y0cC0hSwWAMvzTteuOJRi6LAKcyjjxzN4N/"
"d1+jhaBFmaz0xj6N+rOEjP4CxO47Vm0Vdmm5/0hxPbeJtnjAFRlJwLtJEqhz3mUP"
"ESBot0DEhG/Ql8a4JRrbqICxlQEZHCEDTVv0FzOl64QI2P1/uYWEyiu/E66m1+NL"
"h7EXbNXEpmQuusGbr+x/dOuuuqwphPEIRTopdR84Ceecv1iDh8LucmhOl8/Pfbxw"
"jvj+7rRpZlQeRgvW5q1SEM+RlzNQCQJYr5SARs5SPUWTV5D/KJSMfgsGzOacv3Z0"
"pzhikFoUi/EcVfrV4ppYf6zxC6M7e4o1U7nsoQ33WX32u0BUFpRSxVJ/JIN5sLMx"
"j+yaGgteZGg42ffN6LJXiaQAgwGFz1omUu6/ILkxyf2p28mKzHa0qYJbSFdmRADl"
"7xjzavlKOFH/lFd1qFyKl92PnNQk/lnQ7ObjnelxoMxFp6FkpAXcE65UEAioe8nm"
"nFTGVhHRasuSt4CDOH272OrXpRpXTy06gCIpMeTZxFzXtixIpABqubDQLQIDAQAB"
"o4IBYjCCAV4wgcsGA1UdIwSBwzCBwKGBp6SBpDCBoTELMAkGA1UEBhMCU0cxEjAQ"
"BgNVBAgMCVNpbmdhcG9yZTESMBAGA1UEBwwJU2luZ2Fwb3JlMRYwFAYDVQQKDA1W"
"aXJ0dWFsIFNraWVzMQswCQYDVQQLDAJJVDEgMB4GA1UEAwwXc3ZyLnZpcnR1YWxz"
"a2llcy5jb20uc2cxIzAhBgkqhkiG9w0BCQEWFGphY2t5ODExMDBAeWFob28uY29t"
"ghR3oyXWxq5Pw/Bh1D/upPPoz3rsXjAdBgNVHQ4EFgQUD8UW1uFih6TPivT2Fka9"
"h6enVXkwCQYDVR0TBAIwADALBgNVHQ8EBAMCAvwwHQYDVR0lBBYwFAYIKwYBBQUH"
"AwIGCCsGAQUFBwMBMDgGA1UdEQQxMC+GG3VybjpzdnIudmlydHVhbHNraWVzLmNv"
"bS5zZ4cEwKgBbYIKT1BDU3ZyLTEwOTANBgkqhkiG9w0BAQ0FAAOCAgEAogup344d"
"XV/xAG5CGMFw8dQOU1XA9PjASqRZDJtqtugk33IrQrp5GmyJijDVrZtIS50nKBnu"
"L09O5LKzDIFTIpcwhpVym9hVxZOBvbbox4PvVmjrqBYmchCEl94tTfLepBewgj8M"
"ha+dwwWA6dXhQAjqgkmcEQk71YI1tO7adpXzd76Ab/pLjeOO5VNIWrEQ1eUdZCuR"
"cznCKJ51CsgD3V4FWJp/dB6whIsd7jW9l0VVumUopGy+dKqfnf4MzNaD4xhwo+X5"
"77qjKwnbAllY/4NBtwpNuF/vW8hFR66NmrwLmvRyCK7+VbRbgJxvIktP4B3sGdyz"
"HhoXMFnMGI5UUqm0KNQsT+joi3Xs+l8apN3jVp6wuynweVX6x2ZIxGEUcVV0CrvC"
"geUEMQ1vrBU8JB5U16ezctEMwamVapEN6icUTJoEi4GJ4SLun1IAzlQg3QppAjTS"
"51bHIoXUWCUFkvzIoP98k3EM9TCS33knCLHah5EYQjiCe434aKjWfMa6rzTYCBOA"
"qbHKlJJi435JvA3mXNIzgFh6HOlu4ctToIhO/US3eTT3tPA8yHmI6tWRmr/styKz"
"+Y13cVhsIbdckcx52ZXRRY5heqLQ8GFnsbtC++356pijTUmU5Wq7Q6LsTB/yW0v6"
"FjY/X5+8Bx2P1clIciXlxDQiAZM1punn/fU="
                "\"},"
                /*
                 * the private key for above
                 */
                "{\"self_localhost_key\": \""
"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"
"qgjYOk1ta+W525m9yJinYO5ndWUAPw=="
                "\"}"
          "],"
          "\"trust_stores\": [" /* named cert chains */
                "{"
                        "\"name\": \"le_via_isrg\","
                        "\"stack\": ["
                                "\"isrg_root_x1\""
                        "]"
                "}"
          "],"
          "\"s\": ["
                /*
                 * Client streamtypes
                 */

                "{\"mintest\": {"
                        "\"endpoint\":"		"\"warmcat.com\","
                        "\"port\":"		"443,"
                        "\"protocol\":"		"\"h2\","
                        "\"http_method\":"	"\"GET\","
                        "\"http_url\":"		"\"index.html\","
                        "\"tls\":"		"true,"
                        "\"retry\":"		"\"default\","
                        "\"tls_trust_store\":"	"\"le_via_isrg\""
                "}},"

                /*
                 * This streamtype represents an h2 server listening on :7681,
                 * using a 100-y self-signed tls cert
                 */

                "{\"myserver\": {"
                        /* if given, "endpoint" is network if to bind to */
                        "\"server\":"		"true,"
                        "\"port\":"		"7681,"
                        "\"protocol\":"		"\"h1\","
                        "\"metadata\": [{"
                                "\"mime\": \"Content-Type:\","
                                "\"method\": \"\","
                                "\"path\": \"\""
                        "}],"
                        "\"tls\":"		"true,"
                        /*
                         * A ws server is an http server, if you give a
                         * ws_subprotocol here it's understood we also serve
                         * that ove ws or wss according to tls
                         */
                        "\"ws_subprotocol\":"	"\"mywsprotocol\","
                        "\"server_cert\":"	"\"self_localhost\","
                        "\"server_key\":"	"\"self_localhost_key\""
                "}},"

          "]"
        "}"
;

static int
smd_cb(void *opaque, lws_smd_class_t c, lws_usec_t ts, void *buf, size_t len)
{
        if ((c & LWSSMDCL_SYSTEM_STATE) &&
            !lws_json_simple_strcmp(buf, len, "\"state\":", "OPERATIONAL")) {

                /* create the secure streams */

                lwsl_notice("%s: creating server stream\n", __func__);

                if (lws_ss_create(context, 0, &ssi_server, NULL, NULL,
                                  NULL, NULL)) {
                        lwsl_err("%s: failed to create secure stream\n",
                                 __func__);
                        tests_bad = 1;
                        interrupted = 1;
                        lws_cancel_service(context);
                        return -1;
                }
#if 0
                lwsl_notice("%s: creating client stream\n", __func__);

                if (lws_ss_create(context, 0, &ssi_client, NULL, NULL,
                                  NULL, NULL)) {
                        lwsl_err("%s: failed to create secure stream\n",
                                 __func__);
                        return -1;
                }
#endif
        }

        return 0;
}
