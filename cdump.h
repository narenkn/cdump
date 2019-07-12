#ifndef __CDUMP_DEFINED__
#define __CDUMP_DEFINED__

#include "ffwAPI.h"
#include "simModule.h"
#include <string.h>
#include <vector>
#include <string>
#include <exception>
#include <typeinfo>

#define  CDUMP_PREFIX "cDump"
#define  CDUMP_SCOPE_SEPERATOR "::"

//===============================================
// Global variable definition and declaration
//===============================================

//====================================================
struct sScalarVariable {
//====================================================
  void *obj_ptr;        // actual object ptr 
  byte_T *ptr;          // signals actual ptr 
  char *name;           // signal name
  fsdbVarType type;     // signal type
  void* dt;             // signal data type
  ushort_T lbitnum;     // signal left bit number
  ushort_T rbitnum;     // signal right bit number
  uint32 size;
  fsdbVarDir direction;  // signal direction
  fsdbBytesPerBit bpb;   // signal bytes per bit

  bool force_dump_now;
  bool is_hierpush;
  bool is_hierpop;
  bool is_arraypush;
  bool is_arraypop;
  bool name_malloced;
  bool arrange_values();
  sScalarVariable();
  ~sScalarVariable();

  void init(bool init_it=false, char *n=NULL) {
    if (init_it) {
		  assert(n);
		  name = strdup(n);
		  name_malloced = true;
		}
	}
};


typedef struct sScalarVariable sScalarVariable;

//====================================================
class cDump {
//====================================================
  static cDump  *singleton;
  string fname;
  ffwObject *ffw_obj;
  bool   twi_driver;

public:
  static bool dump_inprog;
  static bool dump_once_done;
  static bool dump_on;
  static uint32 dump_start_cycle;
  static uint32 dump_end_cycle;
  static vector  <sScalarVariable *> wave_vars;
  cDump(string fn = "verilog.fsdb");
  virtual ~cDump();
  virtual void Options();
  virtual void Initial();
  virtual void LateMonitor();
  virtual void Final();
  void StartDump(bool from_twi=true);
  void StopDump();
  static cDump *Ptr(void) { return singleton; }

  template <class T>
  T *getModulePtr() {
    T *ret = NULL;
    for (SimModuleListType::iterator i = SimModulePhaseList->begin(); i != SimModulePhaseList->end(); ++i) {
      try {
        ret = dynamic_cast<T *> (*i);
      } catch (exception &e) {};
			if (ret != NULL)
        break;
    }
    return ret;
  }

  template <class T>
  T *getModulePtr(T *init_module) {
    bool init_module_found = (NULL == init_module) ? false : true;
    T *ret = NULL;
    for (SimModuleListType::iterator i = SimModulePhaseList->begin(); i != SimModulePhaseList->end(); ++i) {
      if ((! init_module_found) && ((void *)*i != (void *) ret)) {
        continue;
      } else
      init_module_found = true;
      try {
        ret = dynamic_cast<T *> (*i);
      } catch (exception &e) {};
      if (ret != NULL)
        break;
    }
    return ret;
  }

};
//====================================================
// Definition of cDUMP macros
// ===================================================
#define cDUMP_ENABLED() \
  Env->GetOptionSwitch(CDUMP_PREFIX)


#define cDUMP_Inst \
  __FUNCTION__ ## cd


#define cDUMP_INIT() \
  cDump *cDUMP_Inst = cDump::Ptr(); \


#define cDUMP_FINISH()


#define cDUMP_Start() \
  cDUMP_Inst->StartDump();


#define cDUMP_EndInternal(pointer) \
  pointer->StopDump();


#define cDUMP_End() \
  cDUMP_EndInternal(cDUMP_Inst)

//Macro for thread to wait on cDump once it starts
#define ThreadWaitOncDumpStart(N) do { \
  uint64 ui1 = N; \
  for (;ui1;ui1--) { \
    ThreadWait(1); \
    if ( cDUMP_Inst->dump_inprog ) \
      break; \
  } \
  if (0 == ui1) { \
    ERROR("ThreadWaitOncDumpStart: Exceed limit of " #N); \
  } \
} while (0)

//Macro for thread wait on cDump once it ends
#define ThreadWaitOncDumpEnd(N) do { \
  uint64 ui1 = N; \
  for (;ui1;ui1--) { \
    ThreadWait(1); \
    if ( ! cDUMP_Inst->dump_inprog ) \
      break; \
  } \
  if (0 == ui1) { \
    ERROR("ThreadWaitOncDumpEnd: Exceed limit of " #N); \
  } \
} while (0)


#define cDUMP_ModulePtr(Module, ptr) \
  Module  *ptr = cDUMP_Inst->getModulePtr<Module>(); \
  if (NULL == ptr) \
  ERROR("cDUMP_ModPtr:Couldn't get pointer for type " #Module);


#define cDUMP_NextModulePtr(Module, ptr) \
  ptr = cDUMP_Inst->getModulePtr<Module>(ptr);


#define cDUMP_ComponentPtr(Component, ID, ptr) \
  Component *ptr = cTestControl::GetTypedComponentPointer<Component>(ID); \
  if (NULL == ptr) { \
    ERROR("cDump:Couldn't get pointer for type" #Component); \
  }

//Macro to push the hierarchy
#define cDUMP_PushHier(hier) do { \
  if (! Env->GetOptionSwitch(CDUMP_PREFIX)) \
    break; \
  sScalarVariable *hier1 = new sScalarVariable;\
  hier1->is_hierpush = true; \
  hier1->name = (char *) strdup(hier); \
  hier1->name_malloced = true; \
  cDump::wave_vars.push_back(hier1); \
} while (0)

//Macro to pull the hierarchy
#define cDUMP_PopHier() do { \
  if (! Env->GetOptionSwitch(CDUMP_PREFIX)) \
    break; \
  sScalarVariable *hier1 = new sScalarVariable;\
  hier1->is_hierpop = true; \
  cDump::wave_vars.push_back(hier1); \
} while (0)

//Macro to push the hierarchy for array data types
#define cDUMP_PushArray(hier, sz) do { \
  if (! Env->GetOptionSwitch(CDUMP_PREFIX)) \
    break; \
  sScalarVariable *hier1 = new sScalarVariable;\
  hier1->is_arraypush = true; \
  hier1->name = (char *) hier; \
  hier1->size = sz; \
  cDump::wave_vars.push_back(hier1); \
} while (0)

//Macro to pull the hierarchy for array data types
#define cDUMP_PopArray() do { \
  if (! Env->GetOptionSwitch(CDUMP_PREFIX)) \
    break; \
  sScalarVariable *hier1 = new sScalarVariable;\
  hier1->is_arraypop = true; \
  cDump::wave_vars.push_back(hier1); \
} while (0)

//Macro for dumping data type int64
#define cDUMP_int64(var, ...) do 		{\
  if (! Env->GetOptionSwitch(CDUMP_PREFIX)) \
    break; \
  sScalarVariable *sig1 = new sScalarVariable;\
  sig1->obj_ptr  = (void *) &(var); \
  sig1->ptr  = (byte_T *) new byte_T[8]; \
  sig1->name = (char *) #var;           \
  sig1->type = FSDB_VT_VCD_PARAMETER; \
  sig1->dt = FSDB_DT_HDL_LONG; \
  sig1->lbitnum = 63;         \
  sig1->rbitnum = 0;          \
  sig1->direction = FSDB_VD_IMPLICIT; \
  sig1->bpb = FSDB_BYTES_PER_BIT_1B;  \
  sig1->init(__VA_ARGS__); \
  if (cDump::dump_inprog) \
    ERROR("Attempt to add dump %s after dump had started...", #var); \
  else \
    cDump::wave_vars.push_back(sig1);   \
} while(0)

//Macro for dumping data type int32
#define cDUMP_int32(var, ...) do 		{\
	if (! Env->GetOptionSwitch(CDUMP_PREFIX)) \
		break; \
  sScalarVariable *sig1 = new sScalarVariable;\
  sig1->obj_ptr  = (void *) &(var); \
  sig1->ptr  = (byte_T *) new byte_T[4]; \
  sig1->name = (char *) #var;           \
  sig1->type = FSDB_VT_VCD_PARAMETER; \
  sig1->dt = FSDB_DT_HDL_INT; \
  sig1->lbitnum = 31;         \
  sig1->rbitnum = 0;          \
  sig1->direction = FSDB_VD_IMPLICIT; \
  sig1->bpb = FSDB_BYTES_PER_BIT_1B;  \
  sig1->init(__VA_ARGS__); \
  if (cDump::dump_inprog) \
    ERROR("Attempt to add dump %s after dump had started...", #var); \
  else \
    cDump::wave_vars.push_back(sig1);   \
} while(0)

//Macro for dumping data type int16
#define cDUMP_int16(var, ...) do 		{\
  if (! Env->GetOptionSwitch(CDUMP_PREFIX)) \
    break; \
  sScalarVariable *sig1 = new sScalarVariable;\
  sig1->obj_ptr  = (void *) &(var); \
  sig1->ptr  = (byte_T *) new byte_T[2]; \
  sig1->name = (char *) #var;           \
  sig1->type = FSDB_VT_VCD_PARAMETER; \
  sig1->dt = FSDB_DT_HDL_INT; \
  sig1->lbitnum = 15;         \
  sig1->rbitnum = 0;          \
  sig1->direction = FSDB_VD_IMPLICIT; \
  sig1->bpb = FSDB_BYTES_PER_BIT_1B;  \
  sig1->init(__VA_ARGS__); \
  if (cDump::dump_inprog) \
    ERROR("Attempt to add dump %s after dump had started...", #var); \
  else \
    cDump::wave_vars.push_back(sig1);   \
} while(0)

//Macro for dumping data type int8
#define cDUMP_int8(var, ...) do 		{\
  if (! Env->GetOptionSwitch(CDUMP_PREFIX)) \
    break; \
  sScalarVariable *sig1 = new sScalarVariable;\
  sig1->obj_ptr  = (void *) &(var); \
  sig1->ptr  = (byte_T *) new byte_T[1]; \
  sig1->name = (char *) #var;           \
  sig1->type = FSDB_VT_VCD_PARAMETER; \
  sig1->dt = FSDB_DT_HDL_INT; \
  sig1->lbitnum = 7;         \
  sig1->rbitnum = 0;          \
  sig1->direction = FSDB_VD_IMPLICIT; \
  sig1->bpb = FSDB_BYTES_PER_BIT_1B;  \
  sig1->init(__VA_ARGS__); \
  if (cDump::dump_inprog) \
    ERROR("Attempt to add dump %s after dump had started...", #var); \
  else \
    cDump::wave_vars.push_back(sig1);   \
} while(0)


//Macro for dumping data type char
#define cDUMP_char(var, ...) cDUMP_int8(var, ##__VA_ARGS__)


//Macro for dumping data type bool
#define cDUMP_bool(var, ...) do 		{\
  if (! Env->GetOptionSwitch(CDUMP_PREFIX)) \
    break; \
  sScalarVariable *sig1 = new sScalarVariable;\
  sig1->obj_ptr  = (void *) &(var); \
  sig1->ptr  = (byte_T *) new byte_T[1]; \
  sig1->name = (char *) #var;           \
  sig1->type = FSDB_VT_VCD_REG; \
  sig1->dt = FSDB_DT_HDL_VERILOG_STANDARD; \
  sig1->lbitnum = 0;         \
  sig1->rbitnum = 0;          \
  sig1->direction = FSDB_VD_IMPLICIT; \
  sig1->bpb = FSDB_BYTES_PER_BIT_1B;  \
  sig1->init(__VA_ARGS__); \
  if (cDump::dump_inprog) \
    ERROR("Attempt to add dump %s after dump had started...", #var); \
  else \
    cDump::wave_vars.push_back(sig1);   \
} while(0)


//Macro for dumping data type float
#define cDUMP_float(var, ...) do 		{\
  if (! Env->GetOptionSwitch(CDUMP_PREFIX)) \
    break; \
  sScalarVariable *sig1 = new sScalarVariable;\
  sig1->obj_ptr  = (void *) &(var); \
  sig1->ptr  = (byte_T *) new double[1]; \
  sig1->name = (char *) #var;           \
  sig1->type = FSDB_VT_VCD_PARAMETER; \
  sig1->dt = FSDB_DT_HANDLE_SV_SHORT_REAL; \
  sig1->lbitnum = 0;         \
  sig1->rbitnum = 0;          \
  sig1->direction = FSDB_VD_IMPLICIT; \
  sig1->bpb = FSDB_BYTES_PER_BIT_8B;  \
  sig1->init(__VA_ARGS__); \
  if (cDump::dump_inprog) \
    ERROR("Attempt to add dump %s after dump had started...", #var); \
  else \
    cDump::wave_vars.push_back(sig1);   \
} while(0)


//Macro for dumping data type double
#define cDUMP_double(var, ...) do 		{\
  if (! Env->GetOptionSwitch(CDUMP_PREFIX)) \
    break; \
  sScalarVariable *sig1 = new sScalarVariable;\
  sig1->obj_ptr  = (void *) &(var); \
  sig1->ptr  = (byte_T *) new double[1]; \
  sig1->name = (char *) #var;           \
  sig1->type = FSDB_VT_VCD_PARAMETER; \
  sig1->dt = FSDB_DT_HANDLE_SV_REAL; \
  sig1->lbitnum = 0;         \
  sig1->rbitnum = 0;          \
  sig1->direction = FSDB_VD_IMPLICIT; \
  sig1->bpb = FSDB_BYTES_PER_BIT_8B;  \
  sig1->init(__VA_ARGS__); \
  if (cDump::dump_inprog) \
    ERROR("Attempt to add dump %s after dump had started...", #var); \
  else \
    cDump::wave_vars.push_back(sig1);   \
} while(0)


//Macro for dumping for any kind of data type
#define cDUMP_var(var, ...) do { \
  if ( (typeid(char) == typeid(var)) || \
      (typeid(uint8) == typeid(var)) || \
      (typeid(int8) == typeid(var)) ) \
    cDUMP_int8(var, ##__VA_ARGS__); \
  else if ((typeid(uint16) == typeid(var)) || (typeid(int16) == typeid(var))) \
    cDUMP_int16(var, ##__VA_ARGS__); \
  else if ((typeid(uint32) == typeid(var)) || (typeid(int32) == typeid(var))) \
    cDUMP_int32(var, ##__VA_ARGS__); \
  else if ((typeid(uint64) == typeid(var)) || (typeid(int64) == typeid(var))) \
    cDUMP_int64(var, ##__VA_ARGS__); \
  else if (typeid(bool) == typeid(var)) \
    cDUMP_bool(var, ##__VA_ARGS__); \
  else if (typeid(float) == typeid(var)) \
    cDUMP_float(var, ##__VA_ARGS__); \
  else if (typeid(double) == typeid(var)) \
    cDUMP_double(var, ##__VA_ARGS__); \
  else \
    ERROR("Attempt to add dump %s for generic int variable not yet supported... Contact Narendran Kumaragurunathan", #var); \
} while(0)

#endif


//Macro for dumping any kind of 1 dimentional data type 
#define cDUMP_Arr1D(var) do { \
  if (! Env->GetOptionSwitch(CDUMP_PREFIX)) \
    break; \
  if (cDump::dump_inprog) { \
    ERROR("Attempt to add dump after dump had started..."); \
    break; \
  } \
  uint32 size = sizeof(var)/sizeof(var[0]); \
  cDUMP_PushArray(#var, size); \
  char c[128]; \
  for (uint32 ui=0; ui<size; ui++) { \
    sprintf(c, "%s[%0d]", #var, ui); \
    cDUMP_var(var[ui], true, c); \
  } \
  cDUMP_PopArray(); \
} while(0)
