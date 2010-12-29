#include "dmzJsExtV8Script.h"
#include <dmzJsModuleV8.h>
#include <dmzRuntimeConfig.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzSystemFile.h>

namespace {

static const dmz::String ScriptCreate ("ScriptCreate");
static const dmz::String ScriptDestroy ("ScriptDestroy");
static const dmz::String InstanceCreate ("InstanceCreate");
static const dmz::String InstanceDestroy ("InstanceDestroy");

};

dmz::JsExtV8Script::JsExtV8Script (const PluginInfo &Info, Config &local) :
      Plugin (Info),
      JsExtV8 (Info),
      JsObserver (Info),
      _log (Info),
      _js (0),
      _core (0) {

   _init (local);
}


dmz::JsExtV8Script::~JsExtV8Script () {

}


// Plugin Interface
void
dmz::JsExtV8Script::update_plugin_state (
      const PluginStateEnum State,
      const UInt32 Level) {

   if (State == PluginStateInit) {

   }
   else if (State == PluginStateStart) {

   }
   else if (State == PluginStateStop) {

   }
   else if (State == PluginStateShutdown) {

   }
}


void
dmz::JsExtV8Script::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

      if (!_js) { _js = JsModule::cast (PluginPtr); }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_js && (_js == JsModule::cast (PluginPtr))) { _js = 0; }
   }
}


// JsExtV8 Interface
void
dmz::JsExtV8Script::update_js_module_v8 (const ModeEnum Mode, JsModuleV8 &module) {

   if (Mode == JsExtV8::Store) { if (!_core) { _core = &module; } }
   else if (Mode == JsExtV8::Remove) { if (&module == _core) { _core = 0; } }
}


void
dmz::JsExtV8Script::update_js_context_v8 (v8::Handle<v8::Context> context) {

   _v8Context = context;
}


void
dmz::JsExtV8Script::update_js_ext_v8_state (const StateEnum State) {

   v8::HandleScope scope;

   if (State == JsExtV8::Register) {

      if (_core) {

         _core->register_interface (
            "dmz/runtime/script",
            _scriptApi.get_new_instance ());
      }
   }
   else if (State == JsExtV8::Init) {

      _scriptStr = V8StringPersist::New (v8::String::NewSymbol ("script"));
      _instanceStr = V8StringPersist::New (v8::String::NewSymbol ("instance"));
      _errorStr = V8StringPersist::New (v8::String::NewSymbol ("error"));
      _stackStr = V8StringPersist::New (v8::String::NewSymbol ("stack"));
   }
   else if (State == JsExtV8::Stop) {

      if (_js) {

         HandleContainerIterator it;
         Handle script (0);

         while (_scripts.get_next (it, script)) { _js->destroy_script (script); }
      }
   }
   else if (State == JsExtV8::Shutdown) {

      _lastError.clear ();

      _scriptStr.Dispose (); _scriptStr.Clear ();
      _instanceStr.Dispose (); _instanceStr.Clear ();
      _errorStr.Dispose (); _errorStr.Clear ();
      _stackStr.Dispose (); _stackStr.Clear ();

      _scriptApi.clear ();
      _v8Context.Clear ();
   }
}


void
dmz::JsExtV8Script::release_js_instance_v8 (
      const Handle InstanceHandle,
      const String &InstanceName,
      v8::Handle<v8::Object> &instance) {

   CallbackStruct *cb = _scriptCreateTable.remove (InstanceHandle);
   if (cb) { delete cb; cb = 0; }

   cb = _scriptDestroyTable.remove (InstanceHandle);
   if (cb) { delete cb; cb = 0; }

   cb = _instanceCreateTable.remove (InstanceHandle);
   if (cb) { delete cb; cb = 0; }

   cb = _instanceDestroyTable.remove (InstanceHandle);
   if (cb) { delete cb; cb = 0; }
}


// JsObserver Interface
void
dmz::JsExtV8Script::update_js_script (
      const JsObserverModeEnum Mode,
      const Handle Module,
      const Handle Script) {

   if (_v8Context.IsEmpty () == false) {

      v8::Context::Scope cscope (_v8Context);
      v8::HandleScope scope;
      HashTableHandleTemplate<CallbackStruct> *table (0);

      if (Mode == JsObserverCreate) { table = &_scriptCreateTable; }
      else if (Mode == JsObserverRelease) { table = &_scriptDestroyTable; }
      else if (Mode == JsObserverDestroy) { table = &_scriptDestroyTable; }

      if (table && _js) {

         const int Argc (5);
         V8Value argv[Argc];

         const String Name = _js->lookup_script_name (Script);
         const String File = _js->lookup_script_file_name (Script);

         argv[0] = v8::String::New (Name ? Name.get_buffer () : "");
         argv[1] = v8::String::New (File ? File.get_buffer () : "");
         argv[2] = v8::Integer::NewFromUnsigned (Script);

         _do_callback (Argc, argv, *table);
      }
   }
}


void
dmz::JsExtV8Script::update_js_instance (
      const JsObserverModeEnum Mode,
      const Handle Module,
      const Handle Instance) {

   if (_v8Context.IsEmpty () == false) {

      v8::Context::Scope cscope (_v8Context);
      v8::HandleScope scope;
      HashTableHandleTemplate<CallbackStruct> *table (0);

      if (Mode == JsObserverCreate) { table = &_instanceCreateTable; }
      else if (Mode == JsObserverRelease) { table = &_instanceDestroyTable; }
      // Ignore Destroy for now.

      if (table && _js) {

         const int Argc (6);
         V8Value argv[Argc];

         const String Name = _js->lookup_instance_name (Instance);
         const Handle ScriptHandle = _js->lookup_instance_script (Instance);
         const String ScriptName = _js->lookup_script_name (ScriptHandle);
         const String File = _js->lookup_script_file_name (ScriptHandle);

         argv[0] = v8::String::New (Name ? Name.get_buffer () : "");
         argv[1] = v8::Integer::NewFromUnsigned (Instance);
         argv[2] = v8::String::New (ScriptName ? Name.get_buffer () : "");
         argv[3] = v8::String::New (File ? File.get_buffer () : "");
         argv[4] = v8::Integer::NewFromUnsigned (ScriptHandle);

         _do_callback (Argc, argv, *table);
      }
   }
}


void
dmz::JsExtV8Script::update_js_error (
      const Handle Module,
      const Handle Script,
      const Handle Instance,
      const String &ErrorMessage,
      const String &StackTrace) {

   if (_js && (_js->get_js_module_handle () == Module)) {

      _lastError.script = Script;
      _lastError.instance = Instance;
      _lastError.error = ErrorMessage;
      _lastError.stack = StackTrace;
   }
}


// JsExtV8Script Interface
// API Bindings
dmz::V8Value
dmz::JsExtV8Script::_script_observe (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsExtV8Script *self = _to_self (Args);

   const int Length = Args.Length ();

   if (self && self->_core) {

      V8Object obj = v8_to_object (Args[0]);
      const String Which = (Length == 2) ? InstanceDestroy : v8_to_string (Args[1]);
      V8Function func = v8_to_function (Args[(Length == 2) ? 1: 2]);

      if (v8_is_valid (obj) && v8_is_valid (func)) {

         const Handle Instance = self->_core->get_instance_handle (obj);

         CallbackStruct *cb = self->_instanceDestroyTable.lookup (Instance);

         if (!cb) {

            cb = new CallbackStruct;
            HashTableHandleTemplate<CallbackStruct> *table (0);

            if (Which == ScriptCreate) { table = &(self->_scriptCreateTable); }
            else if (Which == ScriptDestroy) { table = &(self->_scriptDestroyTable); }
            else if (Which == InstanceCreate) { table = &(self->_instanceCreateTable); }
            else if (Which == InstanceDestroy) { table = &(self->_instanceDestroyTable); }
            else { self->_log.error << "Unknown observer type: " << Which << endl; }

            if (table && cb && table->store (Instance, cb)) {} // do nothing
            else if (cb) { delete cb; cb = 0; }
         }

         if (cb) {

            cb->clear ();
            cb->self = V8ObjectPersist::New (obj);
            cb->func = V8FunctionPersist::New (func);

            result = func;
         }
      }
   }

   return scope.Close (result);
}


dmz::V8Value
dmz::JsExtV8Script::_script_release (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsExtV8Script *self = _to_self (Args);

   if (self && self->_core) {

      const Handle Instance = self->_core->get_instance_handle (Args[0]);
      const int Length = Args.Length ();

      if (Instance) {

         if (Length == 1) {

            static const String NullStr;
            V8Object emptyObj;

            self->release_js_instance_v8 (Instance, NullStr, emptyObj);
            result = v8::Boolean::New (true);
         }
         else if (Length > 1) {

         }
      }
   }

   return scope.Close (result);
}


dmz::V8Value
dmz::JsExtV8Script::_script_error (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsExtV8Script *self = _to_self (Args);

   if (self) {

      V8Object obj = v8::Object::New ();

      obj->Set (self->_scriptStr, v8::Integer::NewFromUnsigned (self->_lastError.script));
      obj->Set (
         self->_instanceStr,
         v8::Integer::NewFromUnsigned (self->_lastError.instance));
      obj->Set (
         self->_errorStr,
         v8::String::New (
            self->_lastError.error ? self->_lastError.error.get_buffer () : "<Unknown>"));
      obj->Set (
         self->_stackStr,
         v8::String::New (
            self->_lastError.stack ? self->_lastError.stack.get_buffer () : "<Unknown>"));

      result = obj;
   }

   return scope.Close (result);
}


dmz::V8Value
dmz::JsExtV8Script::_script_load (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsExtV8Script *self = _to_self (Args);

   if (self && self->_js) {

      const String Name = v8_to_string (Args[0]);
      String fileName;
      if (Args.Length () > 1) { fileName = v8_to_string (Args[1]); }

      if (fileName) { fileName = self->_js->find_script (fileName); }
      else { fileName = self->_js->find_script (Name); }

      if (fileName) {

         FILE *file = open_file (fileName, "rb");

         if (file) {

            const Int32 Size = get_file_size (fileName);

            if (Size > 0) {

               char *buffer = new char[Size];

               if (buffer) {

                  if (read_file (file, Size, buffer) == Size) {

                     const Handle ScriptHandle =
                        self->_js->compile_script (Name, fileName, buffer, Size);

                     if (ScriptHandle) {

                        result = v8::Integer::NewFromUnsigned (ScriptHandle);

                        self->_scripts.add (ScriptHandle);
                     }
                  }

                  delete []buffer; buffer = 0;
               }
            }
            else {

               self->_lastError.clear ();
               self->_lastError.error << "Script: " << fileName << " is empty.";
            }

            close_file (file); file = 0;
         }
         else {

            self->_lastError.clear ();
            self->_lastError.error << "Unable to open script: " << fileName;
         }
      }
      else {

         self->_lastError.clear ();
         self->_lastError.error << "Unable to find script: " << Name;
      }
   }

   return scope.Close (result);
}


dmz::V8Value
dmz::JsExtV8Script::_script_reload (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsExtV8Script *self = _to_self (Args);

   if (self && self->_js) {

      const Handle ScriptHandle = v8_to_handle (Args[0]);

      String fileName = self->_js->lookup_script_file_name (ScriptHandle);

      if (fileName) {

         FILE *file = open_file (fileName, "rb");

         if (file) {

            const Int32 Size = get_file_size (fileName);

            if (Size > 0) {

               char *buffer = new char[Size];

               if (buffer) {

                  if (read_file (file, Size, buffer) == Size) {

                     if (self->_js->recompile_script (ScriptHandle, buffer, Size)) {

                        result = v8::Boolean::New (true);
                     }
                  }

                  delete []buffer; buffer = 0;
               }
            }
            else {

               self->_lastError.clear ();
               self->_lastError.error << "Script: " << fileName << " is empty.";
            }

            close_file (file); file = 0;
         }
         else {

            self->_lastError.clear ();
            self->_lastError.error << "Unable to open script: " << fileName;
         }
      }
      else {

         self->_lastError.clear ();
         self->_lastError.error << "Unable to find script: " << ScriptHandle;
      }
   }

   return scope.Close (result);
}


dmz::V8Value
dmz::JsExtV8Script::_script_compile (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsExtV8Script *self = _to_self (Args);

   if (self && self->_js) {

      Handle scriptHandle (0);
      String name, fileName;

      V8Value value = Args[0];

      if (value->IsString ()) {

         fileName = v8_to_string (value);

         String path, root, ext;

         split_path_file_ext (fileName, path, root, ext);

         scriptHandle = self->_js->lookup_script (name);
      }
      else { scriptHandle = v8_to_handle (Args[0]); }

      if (scriptHandle) { fileName = self->_js->lookup_script_file_name (scriptHandle); }

      String out;
      const char *Buffer (0);
      Int32 size (0);

      V8String str = Args[1]->ToString ();

      if (v8_is_valid (str)) {

         if (str->IsExternalAscii ()) {

            v8::String::ExternalAsciiStringResource *rc =
               str->GetExternalAsciiStringResource ();

            if (rc) {

               Buffer = rc->data ();
               size = rc->length ();
            }
         }
         else {

            out = v8_to_string (str);

            Buffer = out.get_buffer ();
            size = out.get_length ();
         }
      }

      if (Buffer && size) {

         if (scriptHandle) {

            if (self->_js->recompile_script (scriptHandle, Buffer, size)) {

               result = v8::Integer::NewFromUnsigned (scriptHandle);
            }
         }
         else if (fileName) {

            String path, name, ext;

            split_path_file_ext (fileName, path, name, ext);

            if (!name) { name = fileName; }

            scriptHandle = self->_js->compile_script (name, fileName, Buffer, size);

            if (scriptHandle) {

               result = v8::Integer::NewFromUnsigned (scriptHandle);

               self->_scripts.add (scriptHandle);
            }
         }
      }
   }

   return scope.Close (result);
}


dmz::V8Value
dmz::JsExtV8Script::_script_instance (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsExtV8Script *self = _to_self (Args);

   if (self && self->_js) {

      const Handle ScriptHandle = v8_to_handle (Args[0]);
      String name;

      if (Args.Length () > 1) { name = v8_to_string (Args[1]); }
      else { name = self->_js->lookup_script_name (ScriptHandle); }

      if (ScriptHandle && name) {

         Config data;
         const Handle Instance = self->_js->create_instance (name, ScriptHandle, data);

         if (Instance) { result = v8::Integer::NewFromUnsigned (Instance); }
      }
   }

   return scope.Close (result);
}


dmz::V8Value
dmz::JsExtV8Script::_script_destroy (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsExtV8Script *self = _to_self (Args);

   if (self && self->_js) {

      const Handle TheHandle = v8_to_handle (Args[0]);

      if (TheHandle) {

         if (self->_js->lookup_instance_script (TheHandle)) {

            if (self->_js->destroy_instance (TheHandle)) {

               result = v8::Boolean::New (true);
            }
         }
         else if (self->_js->destroy_script (TheHandle)) {

            result = v8::Boolean::New (true);
            self->_scripts.remove (TheHandle);
         }
      }
   }

   return scope.Close (result);
}


void
dmz::JsExtV8Script::_do_callback (
      const int Argc,
      V8Value argv[],
      HashTableHandleTemplate<CallbackStruct> &table) {

   if (_v8Context.IsEmpty () == false) {

      v8::Context::Scope cscope (_v8Context);
      v8::HandleScope scope;

      HashTableHandleIterator it;
      CallbackStruct *cb (0);

      while (table.get_next (it, cb)) {

        v8::TryCatch tc;
        argv[Argc - 1] = v8::Local<v8::Object>::New (cb->self);
        V8Function func = v8::Local<v8::Function>::New (cb->func);

        if (func.IsEmpty () == false) {

           func->Call (cb->self, Argc, argv);

           if (tc.HasCaught ()) {

              if (_core) {

                 _core->handle_v8_exception (it.get_hash_key (), tc);

                 cb = table.remove (it.get_hash_key ());

                 if (cb) { delete cb; cb = 0; }
              }
           }
        }
      }
   }
}


void
dmz::JsExtV8Script::_init (Config &local) {

   v8::HandleScope scope;

   _self = V8ValuePersist::New (v8::External::Wrap (this));

   // Bind API
   _scriptApi.add_constant (ScriptCreate, ScriptCreate);
   _scriptApi.add_constant (ScriptDestroy, ScriptDestroy);
   _scriptApi.add_constant (InstanceCreate, InstanceCreate);
   _scriptApi.add_constant (InstanceDestroy, InstanceDestroy);
   _scriptApi.add_function ("observe", _script_observe, _self);
   _scriptApi.add_function ("release", _script_release, _self);
   _scriptApi.add_function ("error", _script_error, _self);
   _scriptApi.add_function ("load", _script_load, _self);
   _scriptApi.add_function ("reload", _script_reload, _self);
   _scriptApi.add_function ("compile", _script_compile, _self);
   _scriptApi.add_function ("instance", _script_instance, _self);
   _scriptApi.add_function ("destroy", _script_destroy, _self);
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzJsExtV8Script (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::JsExtV8Script (Info, local);
}

};
