var dmz = 
       { module: require('dmz/runtime/module')
       , ui:
          { consts: require('dmz/ui/consts')
          , loader: require('dmz/ui/uiLoader')
          }
       }
   , _exports = {}
   , _form = dmz.ui.loader.load('scripts/comboBox.ui')
   , _comboBox = _form.lookup('comboBox')
   , _main
   ;

(function () {
   _comboBox.addItem("HTTP");
   _comboBox.addItem("FTP");
   _comboBox.addItem("SSH");
   _comboBox.addItem("Telnet");
})();

_comboBox.observe(self, "currentIndexChanged", function (value) {
   _main.print(self.name, "currentIndexChanged:", value, "text:", _comboBox.currentText());
});

dmz.module.subscribe(self, "main", function (Mode, module) {
   
   if (Mode === dmz.module.Activate) {
      
      _main = module
      _main.addPage (_form.title(), _form);
   }
});
