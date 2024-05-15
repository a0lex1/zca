#include <mbedtls/error.h>

namespace asio {
namespace ssl {
namespace mbedtls {

const char *error_message(int error_code)
{
    static char error_buf[100];
    mbedtls_strerror(error_code, error_buf, sizeof(error_buf));
    return error_buf;
}


}}}

