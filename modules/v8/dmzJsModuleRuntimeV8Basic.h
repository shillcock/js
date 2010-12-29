#ifndef DMZ_JS_MODULE_RUNTIME_V8_BASIC_DOT_H
#define DMZ_JS_MODULE_RUNTIME_V8_BASIC_DOT_H

#include <dmzJsExtV8.h>
#include <dmzJsModuleRuntimeV8.h>
#include <dmzJsV8UtilConvert.h>
#include <dmzJsV8UtilHelpers.h>
#include <dmzJsV8UtilTypes.h>
#include <dmzRuntimeData.h>
#include <dmzRuntimeDataConverterTypesBase.h>
#include <dmzRuntimeDefinitions.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimeMessaging.h>
#include <dmzRuntimeObjectType.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeTime.h>
#include <dmzRuntimeUndo.h>
#include <dmzTypesHashTableHandleTemplate.h>

#include <v8.h>

namespace dmz {

   class JsModuleTypesV8;

   class JsModuleRuntimeV8Basic :
         public Plugin,
         public JsModuleRuntimeV8,
         public JsExtV8,
         public UndoObserver {

      public:
         static JsModuleRuntimeV8Basic *to_self (const v8::Arguments &Args);
         JsModuleRuntimeV8Basic (const PluginInfo &Info, Config &local);
         ~JsModuleRuntimeV8Basic ();

         // Plugin Interface
         virtual void update_plugin_state (
            const PluginStateEnum State,
            const UInt32 Level);

         virtual void discover_plugin (
            const PluginDiscoverEnum Mode,
            const Plugin *PluginPtr);

         // JsModuleRuntimeV8 Interface
         virtual v8::Handle<v8::Value> create_v8_config (const Config *Value);
         virtual Boolean to_dmz_config (v8::Handle<v8::Value> value, Config &out);
         virtual v8::Handle<v8::Value> create_v8_data (const Data *Value);
         virtual Boolean to_dmz_data (v8::Handle<v8::Value> value, Data &out);
         virtual v8::Handle<v8::Value> create_v8_event_type (const EventType *Value);
         virtual Boolean to_dmz_event_type (v8::Handle<v8::Value> value, EventType &out);
         virtual v8::Handle<v8::Value> create_v8_log (const String &Name);
         virtual Log *to_dmz_log (v8::Handle<v8::Value> value);
         virtual v8::Handle<v8::Value> create_v8_message (const String &Name);
         virtual v8::Handle<v8::Value> create_v8_object_type (const ObjectType *Value);
         virtual Boolean to_dmz_object_type (
            v8::Handle<v8::Value> value,
            ObjectType &out);

         // JsExtV8 Interface
         virtual void update_js_module_v8 (const ModeEnum Mode, JsModuleV8 &module);
         virtual void update_js_context_v8 (v8::Handle<v8::Context> context);
         virtual void update_js_ext_v8_state (const StateEnum State);

         virtual void release_js_instance_v8 (
            const Handle InstanceHandle,
            const String &InstanceName,
            v8::Handle<v8::Object> &instance);

         // UndoObserver Interface (implemented in dmzJsModuleRuntimeV8BasicUndo.cpp)
         virtual void update_recording_state (
            const UndoRecordingStateEnum RecordingState,
            const UndoRecordingTypeEnum RecordingType,
            const UndoTypeEnum UndoType);

         virtual void update_current_undo_names (
            const String *NextUndoName,
            const String *NextRedoName);

         // JsModuleRuntimeV8Basic Interface
         void handle_v8_exception (const Handle Source, v8::TryCatch &tc);
         // implemented in dmzJsModuleRuntimeV8BasicTimer.cpp
         Boolean delete_timer (V8Object self, V8Function callback);
         Boolean delete_all_timers (V8Object self);

      protected:
         struct MessageStruct;
         struct TimerStruct;
         struct UndoStruct;
         // Static Functions
         // Config bindings implemented in dmzJsModuleRuntimeV8BasicConfig.cpp
         static V8Value _create_config (const v8::Arguments &Args);
         static V8Value _config_is_type_of (const v8::Arguments &Args);
         static V8Value _config_to_string (const v8::Arguments &Args);
         static V8Value _config_attribute (const v8::Arguments &Args);
         static V8Value _config_add (const v8::Arguments &Args);
         static V8Value _config_get (const v8::Arguments &Args);
         static V8Value _config_boolean (const v8::Arguments &Args);
         static V8Value _config_string (const v8::Arguments &Args);
         static V8Value _config_number (const v8::Arguments &Args);
         static V8Value _config_vector (const v8::Arguments &Args);
         static V8Value _config_matrix (const v8::Arguments &Args);
         static V8Value _config_object_type (const v8::Arguments &Args);
         static V8Value _config_event_type (const v8::Arguments &Args);
         static V8Value _config_message (const v8::Arguments &Args);
         static V8Value _config_named_handle (const v8::Arguments &Args);

         // Event Type bindings implemented in dmzJsModuleRuntimeV8BasicEventType.cpp
         static V8Value _event_type_lookup (const v8::Arguments &Args);
         static V8Value _event_type_is_type_of (const v8::Arguments &Args);
         static V8Value _event_type_to_string (const v8::Arguments &Args);
         static V8Value _event_type_get_name (const v8::Arguments &Args);
         static V8Value _event_type_get_handle (const v8::Arguments &Args);
         static V8Value _event_type_get_parent (const v8::Arguments &Args);
         static V8Value _event_type_get_children (const v8::Arguments &Args);
         static V8Value _event_type_is_of_type (const v8::Arguments &Args);
         static V8Value _event_type_is_of_exact_type (const v8::Arguments &Args);
         static V8Value _event_type_get_config (const v8::Arguments &Args);
         static V8Value _event_type_find_config (const v8::Arguments &Args);

         // Data bindings implemented in dmzJsModuleRuntimeV8BasicData.cpp
         static V8Value _create_data (const v8::Arguments &Args);
         static V8Value _data_is_type_of (const v8::Arguments &Args);
         static V8Value _data_wrap_boolean (const v8::Arguments &Args);
         static V8Value _data_unwrap_boolean (const v8::Arguments &Args);
         static V8Value _data_wrap_number (const v8::Arguments &Args);
         static V8Value _data_unwrap_number (const v8::Arguments &Args);
         static V8Value _data_wrap_string (const v8::Arguments &Args);
         static V8Value _data_unwrap_string (const v8::Arguments &Args);
         static V8Value _data_wrap_handle (const v8::Arguments &Args);
         static V8Value _data_unwrap_handle (const v8::Arguments &Args);
         static V8Value _data_to_string (const v8::Arguments &Args);
         static V8Value _data_boolean (const v8::Arguments &Args);
         static V8Value _data_number (const v8::Arguments &Args);
         static V8Value _data_handle (const v8::Arguments &Args);
         static V8Value _data_string (const v8::Arguments &Args);
         static V8Value _data_matrix (const v8::Arguments &Args);
         static V8Value _data_vector (const v8::Arguments &Args);

         // Data bindings implemented in dmzJsModuleRuntimeV8BasicLog.cpp
         static V8Value _create_log (const v8::Arguments &Args);

         // Definitions bindings implemented in dmzJsModuleRuntimeV8BasicDefinitions.cpp
         static V8Value _create_named_handle (const v8::Arguments &Args);
         static V8Value _lookup_named_handle (const v8::Arguments &Args);
         static V8Value _lookup_named_handle_name (const v8::Arguments &Args);
         static V8Value _lookup_state (const v8::Arguments &Args);
         static V8Value _lookup_state_name (const v8::Arguments &Args);

         // Messaging bindings implemented in dmzJsModuleRuntimeV8BasicMessaging.cpp
         static V8Value _create_message (const v8::Arguments &Args);
         static V8Value _message_is_type_of (const v8::Arguments &Args);
         static V8Value _message_global_subscribe (const v8::Arguments &Args);
         static V8Value _message_global_unsubscribe (const v8::Arguments &Args);
         static V8Value _message_to_string (const v8::Arguments &Args);
         static V8Value _message_send (const v8::Arguments &Args);
         static V8Value _message_subscribe (const v8::Arguments &Args);
         static V8Value _message_unsubscribe (const v8::Arguments &Args);

         // Object Type bindings implemented in dmzJsModuleRuntimeV8BasicObjectType.cpp
         static V8Value _object_type_lookup (const v8::Arguments &Args);
         static V8Value _object_type_is_type_of (const v8::Arguments &Args);
         static V8Value _object_type_to_string (const v8::Arguments &Args);
         static V8Value _object_type_get_name (const v8::Arguments &Args);
         static V8Value _object_type_get_handle (const v8::Arguments &Args);
         static V8Value _object_type_get_parent (const v8::Arguments &Args);
         static V8Value _object_type_get_children (const v8::Arguments &Args);
         static V8Value _object_type_is_of_type (const v8::Arguments &Args);
         static V8Value _object_type_is_of_exact_type (const v8::Arguments &Args);
         static V8Value _object_type_get_config (const v8::Arguments &Args);
         static V8Value _object_type_find_config (const v8::Arguments &Args);

         // Timer bindings implemented in dmzJsModuleRuntimeV8BasicTimer.cpp
         static V8Value _set_timer (const v8::Arguments &Args);
         static V8Value _set_repeating_timer (const v8::Arguments &Args);
         static V8Value _set_base_timer (
            const v8::Arguments &Args,
            const Boolean Repeating);
         static V8Value _cancel_timer (const v8::Arguments &Args);
         static V8Value _cancel_all_timers (const v8::Arguments &Args);
         static V8Value _get_frame_delta (const v8::Arguments &Args);
         static V8Value _get_frame_time (const v8::Arguments &Args);
         static V8Value _get_system_time (const v8::Arguments &Args);

         // Undo bindings implemented in dmzJsModuleRuntimeV8BasicUndo.cpp
         static V8Value _undo_reset (const v8::Arguments &Args);
         static V8Value _undo_is_nested_handle (const v8::Arguments &Args);
         static V8Value _undo_is_in_undo (const v8::Arguments &Args);
         static V8Value _undo_is_recording (const v8::Arguments &Args);
         static V8Value _undo_get_type (const v8::Arguments &Args);
         static V8Value _undo_do_next (const v8::Arguments &Args);
         static V8Value _undo_start_record (const v8::Arguments &Args);
         static V8Value _undo_stop_record (const v8::Arguments &Args);
         static V8Value _undo_abort_record (const v8::Arguments &Args);
         static V8Value _undo_store_action (const v8::Arguments &Args);
         static V8Value _undo_observe (const v8::Arguments &Args);
         static V8Value _undo_release (const v8::Arguments &Args);

         // JsModuleRuntimeV8Basic Interface
         Handle _to_handle (V8Value value);
         // implemented in dmzJsModuleRuntimeV8BasicData.cpp
         void _init_config ();
         V8Object _to_config (V8Value value);
         Config *_to_config_ptr (V8Value value);
         void _add_to_config (
            const String &Name,
            const Boolean InArray,
            V8Value value,
            Config &parent);
         // implemented in dmzJsModuleRuntimeV8BasicData.cpp
         void _init_data ();
         V8Object _to_data (V8Value value);
         Data *_to_data_ptr (V8Value value);
         // implemented in dmzJsModuleRuntimeV8BasicDefinitions.cpp
         void _init_definitions ();
         // implemented in dmzJsModuleRuntimeV8BasicEventType.cpp
         void _init_event_type ();
         V8Object _to_event_type (V8Value value);
         EventType *_to_event_type_ptr (V8Value value);
         // implemented in dmzJsModuleRuntimeV8BasicMessaging.cpp
         void _init_messaging ();
         void _reset_messaging ();
         void _release_message_observer (const Handle InstanceHandle);
         Message * _to_message_ptr (V8Value value);
         // implemented in dmzJsModuleRuntimeV8BasicLog.cpp
         void _init_log ();
         // implemented in dmzJsModuleRuntimeV8BasicObjectType.cpp
         void _init_object_type ();
         V8Object _to_object_type (V8Value value);
         ObjectType *_to_object_type_ptr (V8Value value);
         // implemented in dmzJsModuleRuntimeV8BasicTimer.cpp
         void _init_time ();
         void _reset_time ();
         void _release_timer (const Handle InstanceHandle);
         // implemented in dmzJsModuleRuntimeV8BasicUndo.cpp
         void _init_undo ();
         void _reset_undo ();
         void _release_undo_observer (const Handle InstanceHandle);
         // implemented in dmzJsModuleRuntimeV8Basic.cpp
         void _init (Config &local);

         Log _log;
         Time _time;
         Definitions _defs;
         Undo _undo;
         DataConverterBoolean _convertBool;
         DataConverterFloat64 _convertNum;
         DataConverterString _convertStr;
         DataConverterHandle _convertHandle;

         JsModuleV8 *_core;
         JsModuleTypesV8 *_types;

         v8::Handle<v8::Context> _v8Context;

         V8ValuePersist _self;

         V8InterfaceHelper _configApi; 
         V8InterfaceHelper _dataApi; 
         V8InterfaceHelper _defsApi; 
         V8InterfaceHelper _eventTypeApi; 
         V8InterfaceHelper _logApi; 
         V8InterfaceHelper _msgApi; 
         V8InterfaceHelper _objTypeApi; 
         V8InterfaceHelper _timeApi; 
         V8InterfaceHelper _undoApi; 

         HashTableHandleTemplate<MessageStruct> _msgTable;
         HashTableHandleTemplate<TimerStruct> _timerTable;

         HashTableHandleTemplate<UndoStruct> _undoStateTable;
         HashTableHandleTemplate<UndoStruct> _undoNamesTable;

         v8::Persistent<v8::FunctionTemplate> _configFuncTemplate;
         v8::Persistent<v8::Function> _configFunc;

         v8::Persistent<v8::FunctionTemplate> _dataFuncTemplate;
         v8::Persistent<v8::Function> _dataFunc;

         v8::Persistent<v8::FunctionTemplate> _eventTypeFuncTemplate;
         v8::Persistent<v8::Function> _eventTypeFunc;

         v8::Persistent<v8::FunctionTemplate> _logFuncTemplate;
         v8::Persistent<v8::Function> _logFunc;

         v8::Persistent<v8::FunctionTemplate> _msgFuncTemplate;
         v8::Persistent<v8::Function> _msgFunc;

         v8::Persistent<v8::FunctionTemplate> _objTypeFuncTemplate;
         v8::Persistent<v8::Function> _objTypeFunc;

      private:
         JsModuleRuntimeV8Basic ();
         JsModuleRuntimeV8Basic (const JsModuleRuntimeV8Basic &);
         JsModuleRuntimeV8Basic &operator= (const JsModuleRuntimeV8Basic &);

   };
};

inline dmz::JsModuleRuntimeV8Basic *
dmz::JsModuleRuntimeV8Basic::to_self (const v8::Arguments &Args) {

   return (dmz::JsModuleRuntimeV8Basic *)v8::External::Unwrap (Args.Data ());
}

#endif // DMZ_JS_MODULE_RUNTIME_V8_BASIC_DOT_H
