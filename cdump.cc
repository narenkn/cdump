
#ifdef NOVAS_FSDB
#undef NOVAS_FSDB
#endif

#include "env_main.h"
#include "cdump.h"

//====================================================
sScalarVariable::sScalarVariable() :
//====================================================
  force_dump_now(false),
  is_hierpush(false),
  is_hierpop(false),
  is_arraypush(false),
  is_arraypop(false),
  name_malloced(false)
{
}

//====================================================
cDump::cDump (string fn) :
//====================================================
  fname(fn), ffw_obj(NULL) {
  singleton = this;
}

//====================================================
cDump::~cDump () {
//====================================================
  if (ffw_obj != NULL)
    ffw_Close(ffw_obj);
}

//====================================================
void cDump::Options () {
//====================================================
  Env->AddDefaultOption(CDUMP_PREFIX, "OFF");
  Env->AddDefaultOption(CDUMP_PREFIX ".StartCycle", "5");
  Env->AddDefaultOption(CDUMP_PREFIX ".EndCycle", "100000000");
}

//====================================================
void cDump::Initial () {
//====================================================
  dump_on = Env->GetOptionSwitch(CDUMP_PREFIX);
  dump_start_cycle = Env->GetOptionValue(CDUMP_PREFIX ".StartCycle");
  dump_end_cycle = Env->GetOptionValue(CDUMP_PREFIX ".EndCycle");
//	fname = (char *) Env->GetOptionString("DUMP_FILE");
}

//====================================================
void cDump::StopDump () {
//====================================================
  dump_inprog = false;
}

//====================================================
void cDump::StartDump (bool from_twi) {
//====================================================
  twi_driver = from_twi;
	// Look for Dump option 
  if (dump_once_done || dump_inprog)
    return;

  INFO("cDump: Starting Dump of C variables.");
  //
  // Write to the fsdb file
  //
  // Open fsdb file
  ffw_obj = ffw_Open((char *) fname.c_str(), FSDB_FT_VERILOG);
  if (NULL == ffw_obj) {
    ERROR("cDump filed to start");
    return;
  }

  // Use handle scheme for tree creation
  ffw_CreateTreeByHandleScheme(ffw_obj);

  //
  //
  ffwVarMapId var_map_id;
  ffw_SetScopeSeparator(ffw_obj, (char *)CDUMP_SCOPE_SEPERATOR);
  ffw_BeginTree(ffw_obj);
  ffw_CreateScope(ffw_obj, FSDB_ST_VCD_MODULE, (char *) CDUMP_PREFIX);

	
  vector<sScalarVariable *>::iterator it;
  sScalarVariable *var;
  for(it=wave_vars.begin(); it!=wave_vars.end(); it++) {
    var = *it;
    if (var->is_hierpush) {
      ffw_CreateScope(ffw_obj, FSDB_ST_VCD_MODULE, (char *) var->name);
    } else if (var->is_hierpop) {
      ffw_CreateUpscope(ffw_obj);
    } else if (var->is_arraypush) {
      ffw_CreateArrayBegin(ffw_obj, var->name, var->size);
    } else if (var->is_arraypop) {
      ffw_CreateArrayEnd(ffw_obj);
    } else {
      var_map_id = ffw_CreateVarByHandle(ffw_obj, var->type,
        var->direction, (FSDB_DT_HANDLE_SV_SHORT_REAL==var->dt) ? FSDB_DT_HANDLE_SV_REAL : var->dt,
        var->lbitnum, var->rbitnum, var->ptr, var->name, var->bpb);
    }
    var->force_dump_now = true;
  }

  // Tree created
  ffw_EndTree(ffw_obj);

	
  dump_inprog = true;
  dump_once_done = true;
}

//====================================================
void cDump::LateMonitor(void) {
//====================================================
  vector<sScalarVariable *>::iterator it;
  sScalarVariable *var;

	
  if (!dump_inprog && dump_on && (Env->GetCycle() >= dump_start_cycle)) {
    StartDump(false);
  }

	
  if (!dump_inprog)
    return;

  if (!twi_driver && (Env->GetCycle() >= dump_end_cycle)) {
    cDUMP_EndInternal(this);
    cDUMP_FINISH();
    dump_inprog = false;
  }

  for(it=wave_vars.begin(); it!=wave_vars.end(); it++) {
    var = *(it);
    if ((var->is_hierpush) || (var->is_hierpop) || (var->is_arraypush) || (var->is_arraypop))
      continue;
    if (var->arrange_values()) {
//			INFO("cDump::arrange_values:dumping value of variable '%s'", var->name);
      ffw_CreateVarValueByHandle(ffw_obj, var->ptr, (byte_T *) var->ptr);
    }
  }
}


//====================================================
void cDump::Final(void) {
//====================================================
  if (dump_inprog) {
    cDUMP_EndInternal(this);
    cDUMP_FINISH();
    dump_inprog = false;
  }
}

//====================================================
bool sScalarVariable::arrange_values() {
//====================================================
  if (FSDB_VT_VCD_PARAMETER == type) {
    if (FSDB_DT_HANDLE_SV_REAL == dt) {
      if (! force_dump_now)
        if ( ((double *)ptr)[0] == (double) ((double *)obj_ptr)[0] )
          return false;
      ((double *)ptr)[0] = (double) ((double *)obj_ptr)[0];
      force_dump_now = false;
      return true;
    } else if (FSDB_DT_HANDLE_SV_SHORT_REAL == dt) {
      if (! force_dump_now)
        if ( ((double *)ptr)[0] == (double) ((float *)obj_ptr)[0])
          return false;
      ((double *)ptr)[0] = (double) ((float *)obj_ptr)[0];
      force_dump_now = false;
      return true;
    } else if (63 == lbitnum) {
        if (! force_dump_now)
          if ( (((uint32 *)ptr)[0] == (uint32) ((uint32 *)obj_ptr)[1]) &&
               (((uint32 *)ptr)[1] == (uint32) ((uint32 *)obj_ptr)[0]) )
            return false;
        ((uint32 *)ptr)[0] = (uint32) ((uint32 *)obj_ptr)[1];
        ((uint32 *)ptr)[1] = (uint32) ((uint32 *)obj_ptr)[0];
        force_dump_now = false;
        return true;
    } else if (31 == lbitnum) {
        if (! force_dump_now)
          if ( ((uint32 *)ptr)[0] == (uint32) ((uint32 *)obj_ptr)[0] )
            return false;
          ((uint32 *)ptr)[0] = (uint32) ((uint32 *)obj_ptr)[0];
        force_dump_now = false;
      return true;
    } else if (15 == lbitnum) {
        if (! force_dump_now)
          if ( ((uint32 *)ptr)[0] == (uint32) ((uint16 *)obj_ptr)[0] )
            return false;
        ((uint32 *)ptr)[0] = (uint32) ((uint16 *)obj_ptr)[0];
        force_dump_now = false;
        return true;
    } else if (7 == lbitnum) {
        if (! force_dump_now)
          if ( ((uint32 *)ptr)[0] == (uint32) ((uint8 *)obj_ptr)[0] )
            return false;
        ((uint32 *)ptr)[0] = (uint32) ((uint8 *)obj_ptr)[0];
        force_dump_now = false;
        return true;
    }
  } else if (FSDB_VT_VCD_REG == type) {
      if ((0 == lbitnum) && (0 == rbitnum)) {
        if (! force_dump_now)
          if ( ((byte_T *)ptr)[0] == ((bool *)obj_ptr)[0] ? FSDB_BT_VCD_1 : FSDB_BT_VCD_0 )
            return false;
        ((byte_T *)ptr)[0] = ((bool *)obj_ptr)[0] ? FSDB_BT_VCD_1 : FSDB_BT_VCD_0 ;
        force_dump_now = false;
        return true;
    }
  }
  ERROR("cDump::arrange_values:Couldn't dump value of variable '%s'", name);
  return false;
}


//====================================================
sScalarVariable::~sScalarVariable() {
//====================================================
  if (name_malloced) {
    free(name);
  }
  delete [] ptr;
}

vector<sScalarVariable *> cDump::wave_vars;
cDump *cDump::singleton;
bool cDump::dump_inprog = false;
bool cDump::dump_once_done = false;
bool cDump::dump_on = false;
uint32 cDump::dump_start_cycle;
uint32 cDump::dump_end_cycle;
static cDump cdump_inst ("verilog.fsdb");
