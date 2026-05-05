#ifdef __cplusplus
extern "C" {
#endif

#include "mfarray.h"
#include "mfcore.h"

MFArray mfArrayCreate(u64 capacity, u64 elementSize) {
    MF_PANIC_IF(capacity == 0, mfGetLogger(), "The length of the new array can't be allocated and set as 0!");
    MF_PANIC_IF(elementSize == 0, mfGetLogger(), "The element size of the new array can't be allocated and set as 0!");

    MFArray array = {0};
    array.elementSize = elementSize;
    array.capacity = capacity;

    array.data = malloc(elementSize * capacity);
    memset(array.data, 0, elementSize * capacity);

    array.init = true;
    return array;
}

void mfArrayDestroy(MFArray* array) {
    MF_PANIC_IF(array == mfnull, mfGetLogger(), "The array provided shouldn't be 0!");
    MF_PANIC_IF(!array->init, mfGetLogger(), "The array provided isn't initialised!");

    if (array->capacity == 0)
        return;

    MF_FREEMEM(array->data);
    MF_SETMEM(array, 0, sizeof(MFArray));
}

void mfArrayResize(MFArray* array, u64 newCapacity) {
    MF_PANIC_IF(array == mfnull, mfGetLogger(), "The array provided shouldn't be 0!");
    MF_PANIC_IF(!array->init, mfGetLogger(), "The array provided isn't initialised!");
    
    if (newCapacity == 0) {
        MF_FATAL_ABORT(mfGetLogger(), "New capacity cannot be zero!");
    }
    
    if (newCapacity <= array->capacity)
        return;
        
    void* newData = realloc(array->data, array->elementSize * newCapacity);
    if (!newData) {
        MF_FATAL_ABORT(mfGetLogger(), "Failed to reallocate memory for array resize!");
    }
    
    MF_SETMEM((u8*)newData + (array->elementSize * array->capacity), 0, (newCapacity - array->capacity) * array->elementSize);
    
    array->data = newData;
    array->capacity = newCapacity;
}

void mfArrayInsertAt(MFArray* array, u64 index, void* element) {
    MF_PANIC_IF(array == mfnull, mfGetLogger(), "The array provided shouldn't be 0!");
    MF_PANIC_IF(!array->init, mfGetLogger(), "The array provided isn't initialised!");
    MF_DO_IF(index >= array->len, {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The index provided to MFArray is invalid!");
        return;
    });

    if(array->len == array->capacity) {
        mfArrayResize(array, (array->capacity == 0) ? 1 : (array->capacity * 2));
    }

    u64 left = array->len - index;
    void* leftData = MF_ALLOCMEM(void, array->elementSize * left);
    u8* data = (u8*)array->data;
    memcpy(leftData, data + (array->elementSize * index), left * array->elementSize);
    MF_SETMEM(data + (array->elementSize * index), 0, left * array->elementSize);
    memcpy(data + (array->elementSize * index), element, array->elementSize);
    memcpy(data + (array->elementSize * (index + 1)), leftData, left * array->elementSize);

    MF_FREEMEM(leftData);
    array->len++;
}

void mfArrayDeleteAt(MFArray* array, u64 index) {
    MF_PANIC_IF(array == mfnull, mfGetLogger(), "The array provided shouldn't be 0!");
    MF_PANIC_IF(!array->init, mfGetLogger(), "The array provided isn't initialised!");
    MF_DO_IF(index >= array->len, {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The index provided to MFArray is invalid!");
        return;
    });

    u64 left = array->len - index - 1;
    void* leftData = MF_ALLOCMEM(void, array->elementSize * left);
    u8* data = (u8*)array->data;
    memcpy(leftData, data + (array->elementSize * (index + 1)), left * array->elementSize);
    MF_SETMEM(data + (array->elementSize * index), 0, array->elementSize);
    memcpy(data + (array->elementSize * index), leftData, left * array->elementSize);

    MF_FREEMEM(leftData);
    array->len--;
}

#ifdef __cplusplus
}
#endif