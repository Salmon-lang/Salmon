#pragma once

#include "object.h"
#include "vm.h"

ObjFunction *compile(const char *source);
void mark_compiler_roots();
