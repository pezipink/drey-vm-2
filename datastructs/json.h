#ifndef _JSON_H
#define _JSON_H


#include "..\memory\manager.h"
#include "refarray.h"
#include "refhash.h"
#include "..\global.h"


hashref json_to_object(char** data, int len);
stringref object_to_json(memref obj);

#endif

