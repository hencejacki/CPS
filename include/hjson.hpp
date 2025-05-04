#ifndef HJSON_HPP
#define HJSON_HPP
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <climits>
#include <cstdio>

#define HJSON_MAJOR_VERSION 1
#define HJSON_MINOR_VERSION 0
#define HJSON_PATCH_VERSION 1
#define _HJSON_STR(x) #x
#define HJSON_STR(x) _HJSON_STR(x)
#define HJSON_VERSION_STRING \
    HJSON_STR(HJSON_MAJOR_VERSION) "." \
    HJSON_STR(HJSON_MINOR_VERSION) "." \
    HJSON_STR(HJSON_PATCH_VERSION)

#define DBL_EPSILON __DBL_EPSILON__

#define BUFFER_SIZE 32

static const char* ep = nullptr;

enum class ValueType {
    kUnknown = -1,
    kString,
    kNumber,
    kBooleanTrue,
    kBooleanFalse,
    kNull,
    kObject,
    kArray
};

struct HJson {
    struct HJson* next;
    // object/array value data domain
    struct HJson* child;
    ValueType type;
    // integer/boolean value
    int biv;
    // double value
    double dv;
    // string value
    char* sv;
    // key
    char* key;

    HJson* at(int index) {
        if (type != ValueType::kArray) {
            return nullptr;
        }
        HJson* p = child;
        while (p && index--) {
            p = p->next;
        }
        return p;
    }

    HJson* get(const char* k) {
        if (type == ValueType::kString
            || type == ValueType::kNumber
            || type == ValueType::kBooleanFalse
            || type == ValueType::kBooleanTrue
            || type == ValueType::kNull) {
                return 0 == std::strcmp(k, key) ? this : nullptr;
        } else {
            HJson* p = child;
            while (p) {
                if (0 == std::strcmp(k, p->key)) {
                    return p;
                }
                p = p->next;
            }
            return p;
        }
    }

    const char* ToCString() {
        if (type != ValueType::kString && type != ValueType::kNull) {
            perror("Failed converting to string.");
            exit(1);
        }
        return type == ValueType::kNull ? "" : sv;
    }

    int ToInt() {
        if (type != ValueType::kNumber) {
            perror("Failed converting to Int");
            exit(1);
        }
        return biv;
    }

    double ToDouble() {
        if (type != ValueType::kNumber) {
            perror("Failed converting to Int");
            exit(1);
        }
        return dv;
    }

    bool ToBoolean() {
        if (type != ValueType::kBooleanFalse && type != ValueType::kBooleanTrue) {
            perror("Failed converting to Boolean");
            exit(1);
        }
        return biv;
    }
};

struct HJson_buffer {
    char* buffer;
    int offset;
    int size;
};

static const char* HJson_parseValue(HJson* item, const char* value);
static bool HJson_writeValue(HJson *const node, HJson_buffer * const buf);

static const char* HJson_version() {
    return HJSON_VERSION_STRING;
}

static HJson* HJson_new() {
    HJson* node = (HJson*)malloc(sizeof(HJson));
    if (node) {
        memset(node, 0, sizeof(HJson));
    }
    return node;
}

static void HJson_delete(HJson* node) {
    HJson* next;
    while (node) {
        next = node->next;
        if ((node->type == ValueType::kArray 
            || node->type == ValueType::kObject) 
            && node->child) {
            HJson_delete(node->child);
        }
        if (node->type == ValueType::kString && node->sv) {
            free(node->sv);
        }
        if (node->key) {
            free(node->key);
        }
        free(node);
        node = next;
    }
}

static const char* skip(const char* p) {
    while (p && *p && (unsigned char)*p <= 32) {
        p++;
    }
    return p;
}

// Deserialize

static const char* HJson_parseNumber(HJson* item, const char* value) {
    double num = 0;
    int num_sign = 1;
    int scale = 0;
    int exponent = 0;
    int exponent_sign = 1;
    if (*value == '-') {
        num_sign = -1;
        value++;
    }
    // Leading zero?
    while (value && *value == '0') {
        value++;
    }
    // Number?
    if (value && *value >= '1' && *value <= '9') {
        do
        {
            num = num * 10.0 + (*value++ - '0');
        } while (value && *value >= '0' && *value <= '9');
    }
    // Double?
    if (value && *value == '.') {
        value++;
        while (value && *value >= '0' && *value <= '9') {
            num = num * 10.0 + (*value++ - '0');
            scale--;
        }
    }
    // Exponent?
    if (value && (*value == 'e' || *value == 'E')) {
        value++;
        if (value && *value == '-') {
            exponent_sign = -1;
            value++;
        } else if (value && *value == '+') {
            value++;
        }
        while (value && *value >= '0' && *value <= '9') {
            exponent = exponent * 10 + (*value++ - '0');
        }
    }

    num = num_sign * num * pow(10.0, (scale + exponent_sign * exponent));

    item->type = ValueType::kNumber;
    item->dv = num;
    item->biv = static_cast<int>(num);

    return value;
}

// Escape handle
// TODO: Unicode support
static const char* HJson_parseString(HJson* item, const char* value) {
    const char* end_ptr = value + 1;
    int str_len = 0;
    char* sb = 0;
    
    if (value && *value != '\"') {
        ep = value;
        return 0;
    }
    while (end_ptr && *end_ptr != '\"') {
        if (*end_ptr++ == '\\') {
            end_ptr++;
        }
    }
    str_len = end_ptr - value - 1;
    sb = (char*)malloc(str_len + 1);
    if (!sb) {
        return 0;
    }
    item->type = ValueType::kString;
    item->sv = sb;

    // Get string literal
    const char* sp = value + 1;
    char* dp = sb;
    while (sp < end_ptr) {
        *dp++ = *sp++;
    }
    *dp = '\0';
    if (*sp == '\"') sp++;
    return sp;
}

static const char* HJson_parseArray(HJson* item, const char* value) {
    HJson* child;
    if (value && *value != '[') {
        ep = value;
        return 0;
    }
    item->type = ValueType::kArray;
    value = skip(value + 1);
    // Empty array
    if (value && *value == ']') {
        return value + 1;
    }
    item->child = child = HJson_new();

    value = skip(HJson_parseValue(child, skip(value)));
    if (!value) {
        return 0;
    }

    // Comma separator
    while (value && *value == ',') {
        HJson* next;
        next = HJson_new();
        if (!next) {
            return 0;
        }
        child->next = next;
        child = next;

        value = skip(HJson_parseValue(child, skip(value + 1)));
        if (!value) {
            return 0;
        }
    }
    // ending
    if (value && *value == ']') {
        return value + 1;
    }

    ep = value;
    return 0;
}

static const char* HJson_parseObject(HJson* item, const char* value) {
    HJson* child;
    if (value && *value != '{') {
        ep = value;
        return 0;
    }
    item->type = ValueType::kObject;
    value = skip(value + 1);
    if (value && *value == '}') {
        // Empty object
        return value + 1;
    }
    item->child = child = HJson_new();
    // Find key
    value = skip(HJson_parseString(child, skip(value)));
    if (!value) {
        return 0;
    }
    child->key = child->sv;
    child->sv = 0;
    if (value && *value != ':') {
        ep = value;
        return 0;
    }
    // Find value
    value = skip(HJson_parseValue(child, skip(value + 1)));
    if (!value) {
        return 0;
    }
    // Comma separator
    while (value && *value == ',') {
        HJson* next;
        next = HJson_new();
        if (!next) {
            return 0;
        }
        child->next = next;
        child = next;

        // Parse again
        value = skip(HJson_parseString(child, skip(value + 1)));
        if (!value) {
            return 0;
        }

        child->key = child->sv;
        child->sv = 0;
        if (value && *value != ':') {
            ep = value;
            return 0;
        }
        value = skip(HJson_parseValue(child, skip(value + 1)));
        if (!value) {
            return 0;
        }
    }

    // ending
    if (value && *value == '}') {
        return value + 1;
    }

    ep = value;
    return 0;
}

static const char* HJson_parseValue(HJson* item, const char* value) {
    if (!value) return 0;

    if (!strncmp(value, "null", 4)) {
        item->type = ValueType::kNull;
        return value + 4;
    }
    if (!strncmp(value, "false", 5)) {
        item->type = ValueType::kBooleanFalse;
        item->biv = 0;
        return value + 5;
    }
    if (!strncmp(value, "true", 4)) {
        item->type = ValueType::kBooleanTrue;
        item->biv = 1;
        return value + 4;
    }
    if (*value == '\"') {
        return HJson_parseString(item, value);
    }
    if (*value == '-' || (*value >= '0' && *value <= '9')) {
        return HJson_parseNumber(item, value);
    }
    if (*value == '{') {
        return HJson_parseObject(item, value);
    }
    if (*value == '[') {
        return HJson_parseArray(item, value);
    }
    ep = value;
    return 0;
}

static HJson* HJson_parse(const char* value) {
    HJson* root_node = HJson_new();
    if (!root_node) {
        return nullptr;
    }
    const char* end = 0;
    end = HJson_parseValue(root_node, skip(value));
    if (!end) {
        // parse failed
        HJson_delete(root_node);
        root_node = nullptr;
    }
    return root_node;
}

// Serialize

static char* HJson_avoid(HJson_buffer * const p, int needed) {
    char* new_buf = 0;
    int new_size = 0;
    if (!p) {
        return 0;
    }
    if (!p->buffer) {
        p->buffer = (char*)malloc(BUFFER_SIZE);
        p->offset = 0;
        p->size = BUFFER_SIZE;
    }
    needed += p->offset + 1;
    if (needed <= p->size) {
        return p->buffer + p->offset;
    }
    // Simple double diff
    new_size = p->size + (needed - p->size) * 2;
    new_buf = (char*)malloc(new_size);
    if (!new_buf) {
        p->offset = 0;
        p->size = 0;
        return 0;
    }
    memcpy(new_buf, p->buffer, p->size);
    free(p->buffer);
    p->buffer = new_buf;
    p->size = new_size;
    return p->buffer + p->offset;
}

static void HJson_concat(HJson_buffer* const p, const char* v) {
    char* out = 0;
    int v_len = strlen(v);
    out = HJson_avoid(p, v_len);
    if (out) {
        strcpy(out, v);
        p->offset += v_len;
    }
}

static bool HJson_writeNumber(HJson *const node, HJson_buffer * const buf) {
    char* out = 0;
    double dv = node->dv;
    int iv = node->biv;
    if (dv < DBL_EPSILON) {
        // Special zero
        out = (char*)malloc(2);
        if (out) {
            strcpy(out, "0");
        }
    } else if (fabs((double)iv - dv) <= DBL_EPSILON && (dv <= INT_MAX) && (dv >= INT_MIN)) {
        // Integer
        out = (char*)malloc(21);
        if (out) {
            sprintf(out, "%d", iv);
        }
    } else {
        // Floating
        out = (char*)malloc(64);
        if (out) {
            if ((dv * 0) != 0) {
                sprintf(out, "null");
            } else if (fabs(floor(dv) - dv) <= DBL_EPSILON && dv < 1.0e60) {
                sprintf(out, "%0.f", dv);
            } else if (fabs(dv) < 1.0e-6 || fabs(dv) > 1.0e9) {
                sprintf(out, "%e", dv);
            } else {
                sprintf(out, "%f", dv);
            }
        }
    }
    if (out) {
        HJson_concat(buf, out);
        free(out);
        return true;
    }
    free(out);
    return false;
}

static bool HJson_writeString(HJson *const node, HJson_buffer * const buf) {
    const char* inner_sv = node->sv;
    // Begin
    HJson_concat(buf, "\"");
    HJson_concat(buf, inner_sv);
    // End
    HJson_concat(buf, "\"");
    return true;
}   

static bool HJson_writeArray(HJson *const node, HJson_buffer * const buf) {
    // Begin
    HJson_concat(buf, "[");
    HJson* ptr = node->child;
    while (ptr) {
        if (!HJson_writeValue(ptr, buf)) {
            // If failed, write double quotation.
            HJson_concat(buf, "\"Error array\"");
        }
        if (ptr->next) {
            // Comma separator
            HJson_concat(buf, ",");
        }
        ptr = ptr->next;
    }
    // End
    HJson_concat(buf, "]");
    return true;
}

static bool HJson_writeObject(HJson *const node, HJson_buffer * const buf) {
    // Begin
    HJson_concat(buf, "{");
    const char* obj_key = 0;
    HJson* ptr = node->child;
    while (ptr) {
        // Write key
        obj_key = ptr->key;
        HJson_concat(buf, "\"");
        HJson_concat(buf, obj_key);
        HJson_concat(buf, "\"");
        // Write separator
        HJson_concat(buf, ":");
        // Write value
        if (!HJson_writeValue(ptr, buf)) {
            // If failed, write double quotation.
            HJson_concat(buf, "\"Error Object\"");
        }
        // Write separator
        if (ptr->next) {
            HJson_concat(buf, ",");
        }
        ptr = ptr->next;
    }
    //End
    HJson_concat(buf, "}");
    return true;
}

static bool HJson_writeValue(HJson *const node, HJson_buffer * const buf) {
    switch (node->type)
    {
    case ValueType::kArray:
        return HJson_writeArray(node, buf);
    case ValueType::kObject:
        return HJson_writeObject(node, buf);
    case ValueType::kNull:
        HJson_concat(buf, "null");
        return true;
    case ValueType::kBooleanTrue:
        HJson_concat(buf, "true");
        return true;
    case ValueType::kBooleanFalse:
        HJson_concat(buf, "false");
        return true;
    case ValueType::kNumber:
        return HJson_writeNumber(node, buf);
    case ValueType::kString:
        return HJson_writeString(node, buf);
    default:
        return false;
    }
}

static const char* HJson_write(HJson *const node, int& length) {
    if (!node) {
        return 0;
    }
    HJson_buffer* inner_buffer;
    inner_buffer = (HJson_buffer*)malloc(sizeof(HJson_buffer));
    if (!inner_buffer) {
        return 0;
    }
    memset(inner_buffer, 0, sizeof(HJson_buffer));
    if (!HJson_writeValue(node, inner_buffer)) {
        free(inner_buffer->buffer);
        free(inner_buffer);
        return 0;
    }
    // Copy
    char* ret_buf;
    ret_buf = (char*)malloc(inner_buffer->offset + 1);
    if (!ret_buf) {
        return 0;
    }
    length = inner_buffer->offset;
    memset(ret_buf, 0, inner_buffer->offset + 1);
    memcpy(ret_buf, inner_buffer->buffer, inner_buffer->offset + 1);
    free(inner_buffer->buffer);
    free(inner_buffer);
    return ret_buf;
}

static HJson* HJson_createNumber(double v) {
    HJson* node = 0;
    node = HJson_new();
    if (!node) {
        return 0;
    }
    node->type = ValueType::kNumber;
    node->biv = (int)v;
    node->dv = v;
    return node;
}

static HJson* HJson_createBoolean(bool v) {
    HJson* node = 0;
    node = HJson_new();
    if (!node) {
        return 0;
    }
    node->type = v ? ValueType::kBooleanTrue : ValueType::kBooleanFalse;
    node->biv = v ? 0 : 1;
    return node;
}

static HJson* HJson_createString(const char* str) {
    HJson* node = 0;
    node = HJson_new();
    if (!node) {
        return 0;
    }
    node->type = ValueType::kString;
    node->sv = strdup(str);
    return node;
}

static HJson* HJson_createArray(void) {
    HJson* node = 0;
    node = HJson_new();
    if (!node) {
        return 0;
    }
    node->type = ValueType::kArray;
    return node;
}

static HJson* HJson_createObject(void) {
    HJson* node = 0;
    node = HJson_new();
    if (!node) {
        return 0;
    }
    node->type = ValueType::kObject;
    return node;
}

static void HJson_addItem(HJson* container, HJson* item) {
    HJson* child = container->child;
    if (!item) {
        return;
    }
    if (!child) {
        container->child = item;
    } else {
        while (child->next) {
            child = child->next;
        }
        child->next = item;
    }
}

static void HJson_addItemToObject(HJson* container, const char* key, HJson* item) {
    if (!item) {
        return;
    }
    if (item->key) {
        free(item->key);
    }
    item->key = strdup(key);
    HJson_addItem(container, item);
}

static const char* HJson_getError() {
    return ep ? ep : "";
}

#endif // HJSON_HPP