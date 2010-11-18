#ifndef DMZ_JS_PLUGIN_ZIP_LOADER_QT_DOT_H
#define DMZ_JS_PLUGIN_ZIP_LOADER_QT_DOT_H

#include <dmzJsExtV8.h>
#include <dmzJsV8UtilTypes.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimePlugin.h>

namespace dmz {

   class JsPluginZipLoaderQt :
         public Plugin,
         public JsExtV8 {

      public:
         JsPluginZipLoaderQt (const PluginInfo &Info, Config &local);
         ~JsPluginZipLoaderQt ();

         // Plugin Interface
         virtual void update_plugin_state (
            const PluginStateEnum State,
            const UInt32 Level);

         virtual void discover_plugin (
            const PluginDiscoverEnum Mode,
            const Plugin *PluginPtr);

         // JsExtV8 Interface
         virtual void update_js_module_v8 (const ModeEnum Mode, JsModuleV8 &module);
         virtual void update_js_context_v8 (v8::Handle<v8::Context> context);
         virtual void update_js_ext_v8_state (const StateEnum State);

         virtual void release_js_instance_v8 (
            const Handle InstanceHandle,
            const String &InstanceName,
            v8::Handle<v8::Object> &instance);

      protected:
         // JsPluginZipLoaderQt Interface
         void _init (Config &local);

         Log _log;
         v8::Handle<v8::Context> _v8Context;
         JsModuleV8 *_core;


      private:
         JsPluginZipLoaderQt ();
         JsPluginZipLoaderQt (const JsPluginZipLoaderQt &);
         JsPluginZipLoaderQt &operator= (const JsPluginZipLoaderQt &);

   };
};

#endif // DMZ_JS_PLUGIN_ZIP_LOADER_QT_DOT_H
