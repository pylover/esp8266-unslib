#include <osapi.h>

#include "querystring.h"
#include "testing.h"


#define BUFFSIZE    2048
static char result[BUFFSIZE] = {0};

void cb(const char* name, const char* value) {
    os_sprintf(result + strlen(result), "%s: %s, ", name, value);
}


void test_querystring_simple() {
    memset(&result, 0, BUFFSIZE);
    char form[10] = "foo=bar";
    querystring_parse(form, cb);    
    eqstr(result, "foo: bar, ");
}


void test_querystring_long() {
    memset(&result, 0, BUFFSIZE);
    char form[50] = "foo=bar&baz=qux&quux=quuz";
    querystring_parse(form, cb);    
    eqstr(result, "foo: bar, baz: qux, quux: quuz, ");
}


void test_querystring_encoded() {
    memset(&result, 0, BUFFSIZE);
    char form[50] = "foo=%40bar&baz=%3d";
    querystring_parse(form, cb);    
    eqstr(result, "foo: @bar, baz: =, ");
}


int main() {
    test_querystring_simple();
    test_querystring_long();
    test_querystring_encoded();
}
