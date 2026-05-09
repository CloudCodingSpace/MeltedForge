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

    u8* base = (u8*)array->data;
    memmove(base + (index + 1) * array->elementSize, base + index * array->elementSize, (array->len - index) * array->elementSize);

    memcpy(base + index * array->elementSize, element, array->elementSize);
    array->len++;
}

void mfArrayDeleteAt(MFArray* array, u64 index) {
    MF_PANIC_IF(array == mfnull, mfGetLogger(), "The array provided shouldn't be 0!");
    MF_PANIC_IF(!array->init, mfGetLogger(), "The array provided isn't initialised!");
    MF_DO_IF(index >= array->len, {
        slogLogMsg(mfGetLogger(), SLOG_SEVERITY_WARN, "The index provided to MFArray is invalid!");
        return;
    });

    memmove((u8*)array->data + index * array->elementSize, (u8*)array->data + (index + 1) * array->elementSize, (array->len - index - 1) * array->elementSize);

    array->len--;
}

#ifdef __cplusplus
}
#endif