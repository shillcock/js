#include <dmzJsModuleV8.h>
#include "dmzJsPluginZipLoaderQt.h"
#include <dmzJsV8UtilConvert.h>
#include <dmzQtUtil.h>
#include <dmzQtZipFileEngineHandler.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>

dmz::JsPluginZipLoaderQt::JsPluginZipLoaderQt (const PluginInfo &Info, Config &local) :
      Plugin (Info),
      JsExtV8 (Info),
      _log (Info),
      _core (0) {

   _init (local);
}


dmz::JsPluginZipLoaderQt::~JsPluginZipLoaderQt () {

}


// Plugin Interface
void
dmz::JsPluginZipLoaderQt::update_plugin_state (
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
dmz::JsPluginZipLoaderQt::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

   }
   else if (Mode == PluginDiscoverRemove) {

   }
}


// JsExtV8 Interface
void
dmz::JsPluginZipLoaderQt::update_js_module_v8 (const ModeEnum Mode, JsModuleV8 &module) {

   if (Mode == JsExtV8::Store) { if (!_core) { _core = &module; } }
   else if (Mode == JsExtV8::Remove) { if (&module == _core) { _core = 0; } }

}


void
dmz::JsPluginZipLoaderQt::update_js_context_v8 (v8::Handle<v8::Context> context) {

   _v8Context = context;
}


void
dmz::JsPluginZipLoaderQt::update_js_ext_v8_state (const StateEnum State) {

   if (State == JsExtV8::Register) {

   }
   else if (State == JsExtV8::Init) {

   }
   else if (State == JsExtV8::Shutdown) {

      _v8Context.Clear ();
   }
}


void
dmz::JsPluginZipLoaderQt::release_js_instance_v8 (
      const Handle InstanceHandle,
      const String &InstanceName,
      v8::Handle<v8::Object> &instance) {

}


// JsPluginZipLoaderQt Interface
void
dmz::JsPluginZipLoaderQt::_init (Config &local) {

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
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzJsPluginZipLoaderQt (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::JsPluginZipLoaderQt (Info, local);
}

};
