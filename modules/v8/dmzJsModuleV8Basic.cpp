#include <dmzJsExtV8.h>
#include <dmzJsModuleRuntimeV8.h>
#include "dmzJsModuleV8Basic.h"
#include <dmzJsKernelEmbed.h>
#include <dmzJsV8UtilConvert.h>
#include <dmzJsV8UtilStrings.h>
#include <dmzJsV8UtilTypes.h>
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeConfigToStringContainer.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzSystem.h>
#include <dmzTypesHandleContainer.h>
#include <dmzTypesStringContainer.h>
#include <dmzTypesUUID.h>

#include <string.h>
#include <stdio.h>

#include <qdb.h>
static dmz::qdb out;

namespace {

static const char LocalInstanceHeader[] = "(function (self, require) { ";
static const size_t LocalInstanceHeaderLength = strlen (LocalInstanceHeader);
static const char LocalRequireHeader[] = "(function (exports, require) { ";
static const size_t LocalRequireHeaderLength = strlen (LocalRequireHeader);
static const char LocalFooter[] = "\n});";
static const size_t LocalFooterLength = strlen (LocalFooter);

void
local_instance_struct_delete (v8::Persistent<v8::Value> object, void *param) {

   if (param) {

      dmz::JsModuleV8Basic::InstanceStructBase *ptr =
         (dmz::JsModuleV8Basic::InstanceStructBase *)param;

      delete ptr; ptr = 0;
   }

   object.Dispose (); object.Clear ();
}


v8::Handle<v8::Value>
local_print (const v8::Arguments &Args) {

   v8::HandleScope scope;

   dmz::StreamLog *out = (dmz::StreamLog *)v8::External::Unwrap (Args.Data ());

   if (out) {

      const int Length = Args.Length ();

      for (int ix = 0; ix < Length; ix++) {

         if (ix > 0) { *out << " "; }

         *out << dmz::v8_to_string (Args[ix]);
      }

      *out << dmz::endl;
   }

   return scope.Close (v8::Undefined());
}


v8::Handle<v8::Value>
local_create_uuid (const v8::Arguments &Args) {

   v8::HandleScope scope;
   dmz::UUID uuid;
   dmz::create_uuid (uuid);

   return scope.Close (
      v8::String::New (uuid.to_string (dmz::UUID::NotFormatted).get_buffer ()));
}


v8::Handle<v8::Value>
local_require (const v8::Arguments &Args) {

   v8::HandleScope scope;

   dmz::JsModuleV8Basic *module =
      (dmz::JsModuleV8Basic *)v8::External::Unwrap (Args.Data ());

   v8::Handle<v8::Value> result;

   if ((Args.Length () == 1) && module) {

      result = module->require (dmz::v8_to_string (Args[0]));
   }

   if (result.IsEmpty ()) {

      dmz::String msg ("Require unable to resolve: '");
      msg << dmz::v8_to_string (Args[0]) << "'.";

      return dmz::v8_throw (msg);
   }
   else { return scope.Close (result); }
}


v8::Handle<v8::Value>
global_setter (
      v8::Local<v8::String> property,
      v8::Local<v8::Value> value,
      const v8::AccessorInfo &info) {

   v8::HandleScope scope;

   dmz::String err ("Global variables forbidden. Unable to initialize: ");
   err << dmz::v8_to_string (property);

   return v8::ThrowException (v8::Exception::Error (v8::String::New (err.get_buffer ())));
}

};


dmz::JsModuleV8Basic::JsModuleV8Basic (
      const PluginInfo &Info,
      Config &local,
      Config &global) :
      Plugin (Info),
      TimeSlice (Info),
      JsModule (Info),
      JsModuleV8 (Info),
      _log (Info),
      _out ("", LogLevelOut, Info.get_context ()),
      _rc (Info),
      _defs (Info, &_log),
      _shutdown (False),
      _reset (True),
      _runtime (0) {

   _init (local, global);
}


dmz::JsModuleV8Basic::~JsModuleV8Basic () {

   HashTableHandleIterator it;
   InstanceStruct *is (0);

   while (_instanceTable.get_next (it, is)) { _defs.release_unique_name (is->Name); }

   _extTable.clear ();
   _instanceNameTable.clear ();
   _instanceTable.empty ();
   _scriptNameTable.clear ();
   _scriptTable.empty ();

   _globalTemplate.Dispose (); _globalTemplate.Clear ();
   _instanceTemplate.Dispose (); _instanceTemplate.Clear ();
   _requireFuncTemplate.Dispose (); _requireFuncTemplate.Clear ();
   _requireFunc.Dispose (); _requireFunc.Clear ();

   _context.Dispose (); _context.Clear ();

   while (!v8::V8::IdleNotification ()) {;}
   v8::V8::Dispose ();
}


// Plugin Interface
void
dmz::JsModuleV8Basic::update_plugin_state (
      const PluginStateEnum State,
      const UInt32 Level) {

   if (State == PluginStateInit) {

      update_time_slice (0.0);
   }
   else if (State == PluginStateStart) {

   }
   else if (State == PluginStateStop) {

   }
   else if (State == PluginStateShutdown) {

      _shutdown = True;
      _stop_ext ();
      _release_instances ();
      _shutdown_ext ();
   }
}


void
dmz::JsModuleV8Basic::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

      JsExtV8 *ext = JsExtV8::cast (PluginPtr);

      if (ext && _extTable.store (ext->get_js_ext_v8_handle (), ext)) {

         ext->update_js_module_v8 (JsExtV8::Store, *this);

         if (!_context.IsEmpty ()) {

            ext->update_js_context_v8 (_context);
            ext->update_js_ext_v8_state (JsExtV8::Register);
            ext->update_js_ext_v8_state (JsExtV8::Init);
            reset_v8 ();
         }
      }

      if (!_runtime) { _runtime = JsModuleRuntimeV8::cast (PluginPtr); }

      JsObserver *obs = JsObserver::cast (PluginPtr);

      if (obs && _obsTable.store (obs->get_js_observer_handle (), obs)) {

         _dump_to_observer (*obs);
      }
   }
   else if (Mode == PluginDiscoverRemove) {

      JsExtV8 *ext = JsExtV8::cast (PluginPtr);

      if (ext && _extTable.remove (ext->get_js_ext_v8_handle ())) {

         if (!_shutdown) {

            ext->update_js_ext_v8_state (JsExtV8::Stop);
            ext->update_js_ext_v8_state (JsExtV8::Shutdown);
         }

         ext->update_js_module_v8 (JsExtV8::Remove, *this);

         if (!_shutdown) { reset_v8 (); }
      }

      if (_runtime && (_runtime == JsModuleRuntimeV8::cast (PluginPtr))) { _runtime = 0; }

      JsObserver *obs = JsObserver::cast (PluginPtr);
      if (obs) { _obsTable.remove (obs->get_js_observer_handle ()); }
   }
}


#ifdef DMZ_JS_V8_HEAP_DEBUG
namespace { static int last = 0; }
#endif

// TimeSlice Interface
void
dmz::JsModuleV8Basic::update_time_slice (const Float64 DeltaTime) {

   v8::V8::IdleNotification ();

#ifdef DMZ_JS_V8_HEAP_DEBUG
   v8::HeapStatistics hs;
   v8::V8::GetHeapStatistics (&hs);
   const int Size = hs.total_heap_size ();

   if (Size != last) {

      out << Size << " " << Size - last << endl;
      last = Size;
   }
#endif

   if (_reset) {

      _stop_ext ();
      _release_instances ();
      _shutdown_ext ();
      _init_context ();
      v8::Context::Scope cscope (_context);
      _init_ext ();
      _load_scripts ();
      _reset = False;
   }
}

// JsModule Interface
dmz::String
dmz::JsModuleV8Basic::find_script (const String &Name) {

   String result;

   if (!find_file (_localPaths, Name, result)) {

      find_file (_localPaths, Name + ".js", result);
   }

   return result;
}


void
dmz::JsModuleV8Basic::lookup_script_names (StringContainer &list) {

   HashTableHandleIterator it;
   ScriptStruct *script (0);

   while (_scriptTable.get_next (it, script)) { list.add (script->Name); }
}


void
dmz::JsModuleV8Basic::lookup_script_file_names (StringContainer &list) {

   HashTableHandleIterator it;
   ScriptStruct *script (0);

   while (_scriptTable.get_next (it, script)) { list.add (script->FileName); }
}


void
dmz::JsModuleV8Basic::lookup_script_handles (HandleContainer &list) {

   HashTableHandleIterator it;
   ScriptStruct *script (0);

   while (_scriptTable.get_next (it, script)) { list.add (script->ScriptHandle); }
}


dmz::Handle
dmz::JsModuleV8Basic::compile_script (
      const String &Name,
      const String &FileName,
      const char *Script,
      const Int32 Size) {

   Handle result (0);

   ScriptStruct *info = _scriptNameTable.lookup (Name);

   if (!info) {

      info = new ScriptStruct (Name, FileName, get_plugin_runtime_context ());

      if (info && _scriptTable.store (info->ScriptHandle, info)) {

         _scriptNameTable.store (Name, info);
      }
      else if (info) { delete info; info = 0; }
   }

   if (info) {

      if (recompile_script (info->ScriptHandle, Script, Size)) {

         result = info->ScriptHandle;
      }
   }

   return result;
}


dmz::Boolean
dmz::JsModuleV8Basic::recompile_script (
      const Handle ScriptHandle,
      const char *Script,
      const Int32 Size) {

   Boolean result (False);

   ScriptStruct *info = _scriptTable.lookup (ScriptHandle);

   if (info) {

      HashTableHandleIterator inIt;
      InstanceStruct *is (0);

      while (info->table.get_next (inIt, is)) { _shutdown_instance (*is); }

      inIt.reset ();
      while (info->table.get_next (inIt, is)) { _release_instance (*is); }

      V8ExternalString *sptr = new V8ExternalString (
         Script,
         Size,
         LocalInstanceHeader,
         LocalFooter);

      if (info->externalStr) { info->externalStr->Dispose (); info->externalStr = 0; }
      info->externalStr = sptr;
      if (info->externalStr) { info->externalStr->ref (); }

      result = _reload_script (*info);

      inIt.reset ();
      if (result) { while (info->table.get_next (inIt, is)) { _create_instance (*is); } }
   }

   return result;
}


dmz::Boolean
dmz::JsModuleV8Basic::reload_script (const Handle ScriptHandle) {

   Boolean result (False);

   ScriptStruct *info = _scriptTable.lookup (ScriptHandle);

   if (info) {

      HashTableHandleIterator inIt;
      InstanceStruct *is (0);

      while (info->table.get_next (inIt, is)) { _shutdown_instance (*is); }
      while (info->table.get_next (inIt, is)) { _release_instance (*is); }

      result = _reload_script (*info);

      if (result) { while (info->table.get_next (inIt, is)) { _create_instance (*is); } }
   }

   return result;
}


dmz::Boolean
dmz::JsModuleV8Basic::is_script_external (const Handle ScriptHandle) {

   Boolean result (False);

   ScriptStruct *ss = _scriptTable.lookup (ScriptHandle);

   if (ss) { result = ss->externalStr != 0; }

   return result;
}


dmz::Handle
dmz::JsModuleV8Basic::lookup_script (const String &Name) {

   Handle result (0);

   ScriptStruct *script (_scriptNameTable.lookup (Name));

   if (script) { result = script->ScriptHandle; }

   return result;
}


dmz::String
dmz::JsModuleV8Basic::lookup_script_name (const Handle ScriptHandle) {

   String result;

   ScriptStruct *script (_scriptTable.lookup (ScriptHandle));

   if (script) { result = script->Name; }

   return result;
}


dmz::String
dmz::JsModuleV8Basic::lookup_script_file_name (const Handle ScriptHandle) {

   String result;

   ScriptStruct *script (_scriptTable.lookup (ScriptHandle));

   if (script) { result = script->FileName; }

   return result;
}


dmz::Boolean
dmz::JsModuleV8Basic::destroy_script (const Handle ScriptHandle) {

   Boolean result (False);

   ScriptStruct *script = _scriptTable.lookup (ScriptHandle);

   if (script) {

      HashTableHandleIterator it;
      InstanceStruct *instance (0);

      while (script->table.get_next (it, instance)) {

         destroy_instance (it.get_hash_key ());
      }

      script = _scriptTable.lookup (ScriptHandle);

      if (script) {

         _observe_script (JsObserverDestroy, script->ScriptHandle);

         _scriptTable.remove (script->ScriptHandle);
         _scriptNameTable.remove (script->Name);
         delete script; script = 0;
         result = True;
      }
   }

   return result;
}


void
dmz::JsModuleV8Basic::lookup_instance_names (const Handle Script, StringContainer &list) {

   HashTableHandleIterator it;
   InstanceStruct *instance (0);

   while (_instanceTable.get_next (it, instance)) { list.add (instance->Name); }
}


void
dmz::JsModuleV8Basic::lookup_instance_handles (
      const Handle Script,
      HandleContainer &list) {

   HashTableHandleIterator it;
   InstanceStruct *instance (0);

   while (_instanceTable.get_next (it, instance)) { list.add (instance->Object); }
}


dmz::Handle
dmz::JsModuleV8Basic::create_instance (
      const String &Name,
      const Handle ScriptHandle,
      const Config &Init) {

   Handle result (0);

   InstanceStruct *instance = _instanceNameTable.lookup (Name);

   if (!instance) {

      ScriptStruct *ss = _scriptTable.lookup (ScriptHandle);

      if (ss) {

         if (_defs.create_unique_name (Name)) {

            instance = new InstanceStruct (
               Name,
               _defs.create_named_handle (Name),
               *ss);

            if (!_instanceTable.store (instance->Object, instance)) {

               delete instance; instance = 0;
               _log.error << "Script instance name: " << Name
                  << " is not unique." << endl;
            }
            else {

               _instanceNameTable.store (instance->Name, instance);
               instance->local = Init;
            }
         }
         else {

            _log.error << "Unable to register script instance: " << Name
               << ". Because name is not unique." << endl;
         }
      }
      else {

      }
   }

   if (instance) {

      if (recreate_instance (instance->Object, Init)) { result = instance->Object; }
   }

   return result;
}


dmz::Handle
dmz::JsModuleV8Basic::lookup_instance (const String &InstanceName) {

   Handle result (0);

   InstanceStruct *instance (_instanceNameTable.lookup (InstanceName));

   if (instance) { result = instance->Object; }

   return result;
}


dmz::Handle
dmz::JsModuleV8Basic::lookup_instance_script (const Handle Instance) {

   Handle result (0);

   InstanceStruct *instance (_instanceTable.lookup (Instance));

   if (instance) { result = instance->script.ScriptHandle; }

   return result;
}


dmz::String
dmz::JsModuleV8Basic::lookup_instance_name (const Handle Instance) {

   String result;

   InstanceStruct *instance (_instanceTable.lookup (Instance));

   if (instance) { result = instance->Name; }

   return result;
}


dmz::Boolean
dmz::JsModuleV8Basic::lookup_instance_config (const Handle Instance, Config &data) {

   Boolean result (False);

   InstanceStruct *instance (_instanceTable.lookup (Instance));

   if (instance) { data = instance->local; result = True; }

   return result;
}


dmz::Boolean
dmz::JsModuleV8Basic::recreate_instance (const Handle Instance, const Config &Init) {

   Boolean result (False);

   InstanceStruct *is = _instanceTable.lookup (Instance);

   if (is) {

      _shutdown_instance (*is);
      _release_instance (*is);

      is->local = Init;

      if (is) { result = _create_instance (*is); }
   }

   return result;
}


dmz::Boolean
dmz::JsModuleV8Basic::destroy_instance (const Handle Instance) {

   Boolean result (False);

   InstanceStruct *is = _instanceTable.lookup (Instance);

   if (is) {

      _shutdown_instance (*is);
      _release_instance (*is);

      is = _instanceTable.lookup (Instance);

      if (is) {

         _observe_instance (JsObserverDestroy, is->Object);

         _instanceTable.remove (is->Object);
         _instanceNameTable.remove (is->Name);
         _defs.release_unique_name (is->Name);

         delete is; is = 0;
         result = True;
      }
   }

   return result;
}


// JsModuleV8 Interface
void
dmz::JsModuleV8Basic::reset_v8 () { _reset = True; }


v8::Handle<v8::Context>
dmz::JsModuleV8Basic::get_v8_context () { return _context; }


dmz::Boolean
dmz::JsModuleV8Basic::register_interface (            
      const String &Name,
      v8::Persistent<v8::Object> object) {

   Boolean result (False);

   v8::HandleScope scope;
   v8::Persistent<v8::Object> *ptr = new v8::Persistent<v8::Object>;
   *ptr = object;

   if (_requireTable.store (Name, ptr)) { result = True; }
   else {

      delete ptr; ptr = 0;
      _log.error << "Failed to register interface: " << Name
         << ". Name not unique?" << endl;
   }

   return result;
}


v8::Handle<v8::Object>
dmz::JsModuleV8Basic::require (const String &Value) {

   v8::HandleScope scope;

   v8::Handle<v8::Object> result;
   String scriptPath;

   V8Object *ptr = _requireTable.lookup (Value);

   if (!ptr) {

      find_file (_localPaths, Value + ".js", scriptPath);
      if (scriptPath) { ptr = _requireTable.lookup (scriptPath); }
      else {

         find_file (_localPaths, Value, scriptPath);

         if (scriptPath) {

            _log.warn << "Require parameter: " << Value
               << " should not contain the .js suffix" << endl;
            ptr = _requireTable.lookup (scriptPath);
         }
      }
   }

   if (ptr) { result = *ptr; }
   else {

      v8::String::ExternalAsciiStringResource *sptr (0);

      if (scriptPath) {
      
         sptr = new V8FileString (scriptPath, LocalRequireHeader, LocalFooter);
      }
      else if (Value.contains_sub ("dmz/")) {

      }

      if (sptr) {

         _log.info << "Loaded required script: " << scriptPath << endl;
         v8::TryCatch tc;

         v8::Handle<v8::Script> script = v8::Script::Compile (
            v8::String::NewExternal (sptr),
            v8::String::New (scriptPath.get_buffer ()));

         if (tc.HasCaught ()) { handle_v8_exception (0, tc); }
         else {

            V8Value value = script->Run ();

            if (tc.HasCaught ()) { handle_v8_exception (0, tc); }
            else {

               V8Function func = v8_to_function (value);

               if (func.IsEmpty ()) {

                  _log.error << "No function returned from: " << scriptPath << endl;
               }
               else {

                  v8::Persistent<v8::Object> *ptr = new v8::Persistent<v8::Object>;
                  *ptr = v8::Persistent<v8::Object>::New (v8::Object::New ());

                  if (_requireTable.store (scriptPath, ptr)) {

                     result = *ptr;
                     v8::Handle<v8::Value> argv[] = { result, _requireFunc };
                     v8::Handle<v8::Value> value = func->Call (result, 2, argv);

                     if (tc.HasCaught ()) { handle_v8_exception (0, tc); }
                  }
                  else {

                     _log.error << "Failed to store require: " << scriptPath << endl;
                     ptr->Dispose (); ptr->Clear ();
                     delete ptr; ptr = 0;
                  }
               }
            }
         }
      }
      else {

         _log.error << "Failed to load require script: " << Value << endl;
      }
   }

   return result.IsEmpty () ? result : scope.Close (result);
}


void
dmz::JsModuleV8Basic::get_require_list (StringContainer &list) {

   list.clear ();

   HashTableStringIterator it;
   V8ObjectPersist *ptr (0);

   while (_requireTable.get_next (it, ptr)) { list.add (it.get_hash_key ()); }
}


void
dmz::JsModuleV8Basic::handle_v8_exception (const Handle Source, v8::TryCatch &tc) {

   v8::Context::Scope cscope (_context);
   v8::HandleScope hscope;

   const String ExStr = v8_to_string (tc.Exception ());
   String errorStr (ExStr);
   String stackStr;

   v8::Handle<v8::Message> message = tc.Message ();

   if (message.IsEmpty()) {

      _log.error << "Unhandled exception: " << ExStr << endl;
   }
   else {

      const String FileName = v8_to_string (message->GetScriptResourceName ());
      const Int32 Line = message->GetLineNumber ();
      errorStr.flush () << FileName << ":" << Line << ": " << ExStr;
      _log.error << errorStr << endl;

      // Print line of source code.
      _log.error << v8_to_string (message->GetSourceLine ()) << endl;

      int start = message->GetStartColumn ();
      int end = message->GetEndColumn ();

      String space;
      if (start > 1) { space.repeat ("-", start - 1); }
      String line;
      line.repeat ("^", end - start);

      _log.error << (start > 0 ? " " : "") << space << line << endl;

      stackStr = v8_to_string (tc.StackTrace ());

      _log.error << "Stack trace: " << endl << stackStr << endl;
   }

   if (_obsTable.get_count () > 0) {

      const Handle Module (get_plugin_handle ());
      Handle script (0);
      Handle instance (0);

      InstanceStruct *is (_instanceTable.lookup (Source));

      if (is) {

         instance = is->Object;
         script = is->script.ScriptHandle;
      }
      else {

         ScriptStruct *ss (_scriptTable.lookup (Source));

         if (ss) { script = ss->ScriptHandle; }
      }

      HashTableHandleIterator it;
      JsObserver *obs (0);

      while (_obsTable.get_next (it, obs)) {

         obs->update_js_error (Module, script, instance, errorStr, stackStr);
      }
   }
}


dmz::String
dmz::JsModuleV8Basic::get_instance_name (V8Value value) {

   v8::HandleScope scope;

   String result;

   V8Object obj = v8_to_object (value);

   if (obj.IsEmpty () == false) {

      InstanceStructBase *is (0);
      V8Value wrap =  obj->GetHiddenValue (_instanceAttrName);

      if (wrap.IsEmpty () == false) {

         is = (InstanceStructBase *)v8::External::Unwrap (wrap);
      }

      if (is) { result = is->Name; }
   }

   return result;
}


dmz::Handle
dmz::JsModuleV8Basic::get_instance_handle (V8Value value) {

   v8::HandleScope scope;

   Handle result (0);

   V8Object obj = v8_to_object (value);

   if (obj.IsEmpty () == false) {

      InstanceStructBase *is (0);
      V8Value wrap =  obj->GetHiddenValue (_instanceAttrName);

      if (wrap.IsEmpty () == false) {

         is = (InstanceStructBase *)v8::External::Unwrap (wrap);
      }

      if (is) { result = is->Object; }
   }

   return result;
}


dmz::Boolean
dmz::JsModuleV8Basic::set_external_instance_handle_and_name (
      const Handle TheHandle,
      const String &TheName,
      v8::Handle<v8::Value> value) {

   Boolean result (False);

   v8::HandleScope scope;

   V8Object obj = v8_to_object (value);

   if (obj.IsEmpty () == false) {

      InstanceStructBase *is (0);
      V8Value wrap =  obj->GetHiddenValue (_instanceAttrName);

      if (wrap.IsEmpty () == false) {

         is = (InstanceStructBase *)v8::External::Unwrap (wrap);
      }

      if (!is) {

         is = new InstanceStructBase (TheName, TheHandle);

         if (is) {

            obj->SetHiddenValue (_instanceAttrName, v8::External::Wrap ((void *)is));
            V8ObjectPersist dtor = V8ObjectPersist::New (obj);
            dtor.MakeWeak ((void *)is, local_instance_struct_delete); 
            result = True;
         }
      }
   }

   return result;
}


dmz::V8Value
dmz::JsModuleV8Basic::_create_self (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();
   JsModuleV8Basic *self = (JsModuleV8Basic *)v8::External::Unwrap (Args.Data ());

   if (self) {

      const String Name = v8_to_string (Args[0]);

      if (Name) {

         const Handle Instance = self->_defs.create_named_handle (Name);

         V8Object obj = v8::Object::New ();

         if (self->set_external_instance_handle_and_name (Instance, Name, obj)) {

            obj->Set (
               v8::String::NewSymbol ("name"),
               v8::String::New (Name.get_buffer ()));

            result = obj;
         }
      }
   }

   return scope.Close (result);
}


void
dmz::JsModuleV8Basic::_dump_to_observer (JsObserver &obs) {

   const Handle Module (get_plugin_handle ());

   HashTableHandleIterator it;
   ScriptStruct *ss (0);

   while (_scriptTable.get_next (it, ss)) {

      if (ss->ctor.IsEmpty () == False) {

         obs.update_js_script (JsObserverCreate, Module, ss->ScriptHandle);
      }
   }

   it.reset ();
   InstanceStruct *is (0);

   while (_instanceTable.get_next (it, is)) {

      if (is->self.IsEmpty () == False) {

         obs.update_js_instance (JsObserverCreate, Module, is->Object);
      }
   }
}


void
dmz::JsModuleV8Basic::_observe_script (
      const JsObserverModeEnum Mode,
      const Handle Script) {

   const Handle Module (get_plugin_handle ());
   HashTableHandleIterator it;
   JsObserver *obs (0);

   while (_obsTable.get_next (it, obs)) { obs->update_js_script (Mode, Module, Script); }
}


void
dmz::JsModuleV8Basic::_observe_instance (
      const JsObserverModeEnum Mode,
      const Handle Instance) {

   const Handle Module (get_plugin_handle ());
   HashTableHandleIterator it;
   JsObserver *obs (0);

   while (_obsTable.get_next (it, obs)) {

      obs->update_js_instance (Mode, Module, Instance);
   }
}


void
dmz::JsModuleV8Basic::_empty_require () {

   v8::HandleScope scope;

   HashTableStringIterator it;
   v8::Persistent<v8::Object> *ptr (0);

   while (_requireTable.get_next (it, ptr)) { ptr->Dispose (); ptr->Clear (); }

   _requireTable.empty ();
}


void
dmz::JsModuleV8Basic::_release_instances () {

   if (_context.IsEmpty () == false) {

      v8::Context::Scope cscope (_context);
      v8::HandleScope scope;

      HashTableHandleIterator it;
      InstanceStruct *is (0);

      while (_instanceTable.get_next (it, is)) { _shutdown_instance (*is); }

      it.reset ();
      is = 0;

      while (_instanceTable.get_next (it, is)) {

         _release_instance (*is);
      }
   }
}


void
dmz::JsModuleV8Basic::_init_context () {

   v8::HandleScope scope;

   _empty_require ();

   _requireFunc.Dispose (); _requireFunc.Clear ();
   _instanceAttrName.Dispose (); _instanceAttrName.Clear ();
   _shutdownFuncName.Dispose (); _shutdownFuncName.Clear ();

   if (_globalTemplate.IsEmpty ()) {

      _globalTemplate = v8::Persistent<v8::ObjectTemplate>::New (
         v8::ObjectTemplate::New ());

      _globalTemplate->SetNamedPropertyHandler (0, global_setter);
   }

   if (_instanceTemplate.IsEmpty ()) {

      _instanceTemplate = v8::Persistent<v8::ObjectTemplate>::New (
         v8::ObjectTemplate::New ());
   }

   if (_requireFuncTemplate.IsEmpty ()) {

      _requireFuncTemplate = v8::Persistent<v8::FunctionTemplate>::New (
         v8::FunctionTemplate::New (local_require, v8::External::Wrap (this)));
   }

//   char flags[] = "--expose_debug_as V8";
//   v8::V8::SetFlagsFromString (flags, strlen (flags));
   while (!v8::V8::IdleNotification ()) {;}

   if (_context.IsEmpty () == false) {

      _context.Dispose (); _context.Clear ();
      v8::V8::ContextDisposedNotification ();
   }

   _context = v8::Context::New (0, _globalTemplate);

   v8::Context::Scope cscope (_context);

   _instanceAttrName = V8StringPersist::New (v8::String::NewSymbol ("dmz::InstanceInfo"));

   _shutdownFuncName = V8StringPersist::New (v8::String::NewSymbol ("shutdown"));

   _init_builtins ();

   _requireFunc = v8::Persistent<v8::Function>::New (
      _requireFuncTemplate->GetFunction ());
}


void
dmz::JsModuleV8Basic::_init_builtins () {

   v8::HandleScope scope;

   v8::Persistent<v8::Object> *ptr = new v8::Persistent<v8::Object>;

   if (ptr) {

      *ptr= v8::Persistent<v8::Object>::New (v8::Object::New ());

      (*ptr)->Set (
         v8::String::NewSymbol ("puts"),
         v8::FunctionTemplate::New (
            local_print,
            v8::External::Wrap (&_out))->GetFunction ());

      (*ptr)->Set (
         v8::String::NewSymbol ("createUUID"),
         v8::FunctionTemplate::New (local_create_uuid)->GetFunction ());

      (*ptr)->Set (
         v8::String::NewSymbol ("createSelf"),
         v8::FunctionTemplate::New (
            _create_self,
            v8::External::Wrap (this))->GetFunction ());

      if (!_requireTable.store ("sys", ptr)) {

         ptr->Dispose (); ptr->Clear ();
         delete ptr; ptr = 0;
      }
   }
}


void
dmz::JsModuleV8Basic::_init_ext () {

   v8::HandleScope scope;

   HashTableHandleIterator it;
   JsExtV8 *ext (0);

   while (_extTable.get_next (it, ext)) {

      ext->update_js_context_v8 (_context);
      ext->update_js_ext_v8_state (JsExtV8::Register);
   }

   it.reset ();
   ext = 0;

   while (_extTable.get_next (it, ext)) {

      ext->update_js_ext_v8_state (JsExtV8::Init);
   }
}


void
dmz::JsModuleV8Basic::_stop_ext () {

   if (_context.IsEmpty () == false) {

      v8::Context::Scope cscope (_context);
      v8::HandleScope scope;

      HashTableHandleIterator it;
      JsExtV8 *ext (0);

      while (_extTable.get_next (it, ext)) {

         ext->update_js_ext_v8_state (JsExtV8::Stop);
      }
   }
}


void
dmz::JsModuleV8Basic::_shutdown_ext () {

   if (_context.IsEmpty () == false) {

      v8::Context::Scope cscope (_context);
      v8::HandleScope scope;

      HashTableHandleIterator it;
      JsExtV8 *ext (0);

      while (_extTable.get_next (it, ext)) {

         ext->update_js_ext_v8_state (JsExtV8::Shutdown);
      }
   }
}


dmz::Boolean
dmz::JsModuleV8Basic::_reload_script (ScriptStruct &info) {

   Boolean result (False);

   if (info.script.IsEmpty () == false) {

      _observe_script (JsObserverRelease, info.ScriptHandle);
   }

   info.clear ();
      
   v8::TryCatch tc;

   v8::String::ExternalAsciiStringResource *sptr (0);

   if (info.externalStr) {

      sptr = info.externalStr;
      info.externalStr->ref ();
   }
   else {

      sptr = new V8FileString (info.FileName, LocalInstanceHeader, LocalFooter);
   }

   info.compileCount++;

   v8::Handle<v8::Script> script = v8::Script::Compile (
      v8::String::NewExternal (sptr),
      v8::String::New (info.FileName.get_buffer ()));

   if (tc.HasCaught ()) { handle_v8_exception (info.ScriptHandle, tc); }
   else {

      _log.info << "Loaded script: " << info.FileName << endl;

      info.script = v8::Persistent<v8::Script>::New (script);
      v8::Handle<v8::Value> value = info.script->Run ();

      if (tc.HasCaught ()) { handle_v8_exception (info.ScriptHandle, tc); }
      else {

         V8Function func = v8_to_function (value);
         if (func.IsEmpty ()) {
            // Error! no function returned.
            _log.error << "No function returned from: " << info.FileName << endl;
         }
         else {

            info.ctor = v8::Persistent<v8::Function>::New (func);
            _observe_script (JsObserverCreate, info.ScriptHandle);
            result = True;
         }
      }
   }

   return result;
}


void
dmz::JsModuleV8Basic::_load_scripts () {

   v8::Context::Scope cscope (_context);
   v8::HandleScope hscope;

   HashTableHandleIterator it;
   ScriptStruct *info (0);

   while (_scriptTable.get_next (it, info)) { _reload_script (*info); }

   it.reset ();
   InstanceStruct *instance (0);

   while (_instanceTable.get_next (it, instance)) { _create_instance (*instance); }
}


dmz::Boolean
dmz::JsModuleV8Basic::_create_instance (InstanceStruct &instance) {

   Boolean result (False);

   if ((_context.IsEmpty () == false) &&
         (instance.compileCount != instance.script.compileCount)) {

      instance.compileCount = instance.script.compileCount;

      v8::Context::Scope cscope (_context);
      v8::HandleScope scope;

      if (instance.script.ctor.IsEmpty () == false) {

         v8::TryCatch tc;

         if (instance.self.IsEmpty () == false) {

            instance.self.Dispose (); instance.self.Clear ();
         }

         instance.self =
            v8::Persistent<v8::Object>::New (_instanceTemplate->NewInstance ());

         InstanceStructBase *isb = &instance;

         instance.self->SetHiddenValue (
            _instanceAttrName,
            v8::External::Wrap ((void *)isb));

         instance.self->Set (
            v8::String::NewSymbol ("name"),
            v8::String::New (instance.Name.get_buffer ()));

         if (_runtime) {

            instance.self->Set (
               v8::String::NewSymbol ("config"),
               _runtime->create_v8_config (&(instance.local)));

            instance.self->Set (
               v8::String::NewSymbol ("log"),
               _runtime->create_v8_log (instance.Name));
         }

         v8::Handle<v8::Value> argv[] = { instance.self, _requireFunc };

         V8Value value = instance.script.ctor->Call (instance.self, 2, argv);

         if (tc.HasCaught ()) { handle_v8_exception (instance.Object, tc); }
         else {

            _log.info << "Created instance: " << instance.Name
               << " from script: " << instance.script.FileName << endl;

            _observe_instance (JsObserverCreate, instance.Object);

            result = True;
         }
      }
   }

   return result;
}


void
dmz::JsModuleV8Basic::_shutdown_instance (InstanceStruct &instance) {

   if (_context.IsEmpty () == false) {

      v8::Context::Scope cscope (_context);
      v8::HandleScope scope;

      if (instance.self.IsEmpty () == false) {

         V8Function func = v8_to_function (instance.self->Get (_shutdownFuncName));

         if (func.IsEmpty () == false) {

            v8::TryCatch tc;
            v8::Handle<v8::Value> argv[] = { instance.self };
            func->Call (instance.self, 1, argv);
            if (tc.HasCaught ()) { handle_v8_exception (instance.Object, tc); }
         }
      }
   }
}


void
dmz::JsModuleV8Basic::_release_instance (InstanceStruct &instance) {

   if (instance.self.IsEmpty () == false) {

      _observe_instance (JsObserverRelease, instance.Object);

      HashTableHandleIterator it;
      JsExtV8 *ext (0);

      while (_extTable.get_next (it, ext)) {

         ext->release_js_instance_v8 (instance.Object, instance.Name, instance.self);
      }

      if (_context.IsEmpty () == false) {

         v8::Context::Scope cscope (_context);
         v8::HandleScope scope;
         instance.self.Dispose (); instance.self.Clear ();
      }
   }
}


dmz::JsModuleV8Basic::ScriptStruct *
dmz::JsModuleV8Basic::_find_script (Config &script) {

   const String Name = config_to_string ("name", script);

   ScriptStruct *result (_scriptNameTable.lookup (Name));

   if (!result && Name) {

      String scriptPath = _rc.find_file (Name);

      if (!scriptPath && (_localPaths.get_count () > 0)) {

         find_file (_localPaths, Name + ".js", scriptPath);
      }

      if (scriptPath) {

         result = _scriptNameTable.lookup (scriptPath);

         if (!result) {

            result = new ScriptStruct (Name, scriptPath, get_plugin_runtime_context ());

            if (result && _scriptTable.store (result->ScriptHandle, result)) {

               _scriptNameTable.store (Name, result);
            }
            else {

               delete result; result = 0;
               _log.error << "Failed to add script: " << scriptPath << endl
                  << "Has script already been specified?" << endl;
            }
         }
      }
      else {

         _log.error << "Failed to find script resource: " << Name << endl;
      }
   }

   return result;
}


void
dmz::JsModuleV8Basic::_init (Config &local, Config &global) {

   _localPaths = config_to_path_string_container (local);

   Config scriptList;

   if (local.lookup_all_config ("script", scriptList)) {

      ConfigIterator it;
      Config script;

      while (scriptList.get_next_config (it, script)) { _find_script (script); }
   }

   Config instanceList;

   if (local.lookup_all_config ("instance", instanceList)) {

      ConfigIterator it;
      Config instance;

      while (instanceList.get_next_config (it, instance)) {

         ScriptStruct *ss = _find_script (instance);

         if (ss) {

            const String UniqueName = config_to_string ("unique", instance, ss->Name);
            const String ScopeName = config_to_string ("scope", instance, UniqueName);

            if (_defs.create_unique_name (UniqueName)) {

               InstanceStruct *ptr = new InstanceStruct (
                  UniqueName,
                  _defs.create_named_handle (UniqueName),
                  *ss);

               if (!_instanceTable.store (ptr->Object, ptr)) {

                  delete ptr; ptr = 0;
                  _log.error << "Script instance name: " << UniqueName
                     << " is not unique." << endl;
               }
               else {

                  _instanceNameTable.store (ptr->Name, ptr);
                  Config local (ScopeName);
                  global.lookup_all_config_merged (String ("dmz.") + ScopeName, local);
                  ptr->local = local;
               }
            }
            else {

               _log.error << "Unable to register script instance: " << UniqueName
                  << ". Because name is not unique." << endl;
            }
         }
      }
   }

   _log.info << "Using V8 v" << v8::V8::GetVersion () << endl;
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzJsModuleV8Basic (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::JsModuleV8Basic (Info, local, global);
}

};
