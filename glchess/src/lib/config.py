# -*- coding: utf-8 -*-
from defaults import *

try:
    import mateconf
    import gobject
except ImportError:
    haveMateConfSupport = False
    _notifiers = {}
    _values = {}

    import xml.dom.minidom
    document = None
    try:
        document = xml.dom.minidom.parse(CONFIG_FILE)
    except IOError:
        pass
    except xml.parsers.expat.ExpatError:
        print 'Configuration at ' + CONFIG_FILE + ' is invalid, ignoring'
    else:
        print 'Loading configuration from ' + CONFIG_FILE

    def _bool(string):
        return string == 'True'
        
    valueTypes = {'int': int, 'bool': _bool, 'float': float, 'str': str}
    
    if document is not None:
        elements = document.getElementsByTagName('config')
        
        for e in elements:
            for n in e.getElementsByTagName('value'):
                try:
                    name = n.attributes['name'].nodeValue
                except KeyError:
                    continue
                try:
                    valueType = n.attributes['type'].nodeValue
                except KeyError:
                    continue
                if len(n.childNodes) != 1 or n.childNodes[0].nodeType != n.TEXT_NODE:
                    continue
                valueString = n.childNodes[0].nodeValue
                
                try:
                    value = valueTypes[valueType](valueString)
                except KeyError:
                    continue

                _values[name] = value

else:
    haveMateConfSupport = True
    _MATECONF_DIR = '/apps/glchess/'
    _config = mateconf.client_get_default()
    try:
        _config.add_dir(_MATECONF_DIR[:-1], mateconf.CLIENT_PRELOAD_NONE)
    except gobject.GError:
        pass
    
    _mateconfGetFunction = {mateconf.VALUE_BOOL: mateconf.Value.get_bool,
                         mateconf.VALUE_FLOAT: mateconf.Value.get_float,
                         mateconf.VALUE_INT: mateconf.Value.get_int,
                         mateconf.VALUE_STRING: mateconf.Value.get_string}
                         
    _mateconfSetFunction = {bool:    _config.set_bool,
                         float:   _config.set_float,
                         int:     _config.set_int,
                         str:     _config.set_string,
                         unicode: _config.set_string}
              
# Config default values
_defaults = {'show_toolbar':                     True,
             'show_history':                     True,
             'maximised':                        False,
             'fullscreen':                       False,
             'piece_style':                      'simple',
             'show_3d':                          False,
             'show_3d_smooth':                   False,             
             'show_move_hints':                  True,
             'move_format':                      'human',
             'promotion_type':                   'queen',
             'board_view':                       'human',
             'show_comments':                    False,
             'show_numbering':                   False,
             'enable_networking':                True,
             'load_directory':                   '',
             'save_directory':                   '',
             'new_game_dialog/move_time':        0,
             'new_game_dialog/white/type':       '',
             'new_game_dialog/white/difficulty': '',
             'new_game_dialog/black/type':       '',
             'new_game_dialog/black/difficulty': ''}

class Error(Exception):
    """Exception for configuration use"""
    pass

def get(name):
    """Get a configuration value.
    
    'name' is the name of the value to get (string).
    
    Raises an Error exception if the value does not exist.
    """
    if haveMateConfSupport:
        try:
            entry = _config.get(_MATECONF_DIR + name)
        except gobject.GError:
            entry = None
        
        if entry is None:
            try:
                return _defaults[name]
            except KeyError:
                raise Error('No config value: ' + repr(name))

        try:
            function = _mateconfGetFunction[entry.type]
        except KeyError:
            raise Error('Unknown value type')
        
        return function(entry)
        
    else:
        try:
            return _values[name]
        except KeyError:
            try:
                return _defaults[name]
            except KeyError:
                raise Error('No config value: ' + repr(name))

def set(name, value):
    """Set a configuration value.
    
    'name' is the name of the value to set (string).
    'value' is the value to set to (int, str, float, bool).
    """
    if haveMateConfSupport:
        try:
            function = _mateconfSetFunction[type(value)]
        except KeyError:
            raise TypeError('Only config values of type: int, str, float, bool supported')
        else:
            try:
                function(_MATECONF_DIR + name, value)
            except gobject.GError:
                pass

    else:
        # Debounce
        try:
            oldValue = _values[name]
        except KeyError:
            pass
        else:
            if oldValue == value:
                return
        
        # Use new value and notify watchers
        _values[name] = value
        try:
            watchers = _notifiers[name]
        except KeyError:
            pass
        else:
            for func in watchers:
                func(name, value)
                
        # Save configuration
        _save()
        
def default(name):
    set(name, _defaults[name])
                
def _watch(client, _, entry, (function, name)):
    value = get(name)
    function(name, value)

def watch(name, function):
    """
    """
    if haveMateConfSupport:
        try:
            _config.notify_add(_MATECONF_DIR + name, _watch, (function, name))
        except gobject.GError:
            pass
    else:
        try:
            watchers = _notifiers[name]
        except KeyError:
            watchers = _notifiers[name] = []            
        watchers.append(function)

def _save():
    """Save the current configuration"""
    if haveMateConfSupport:
        return
    
    document = xml.dom.minidom.Document()
    
    e = document.createComment('Automatically generated by glChess, do not edit!')
    document.appendChild(e)
    
    root = document.createElement('config')
    document.appendChild(root)
    
    valueNames = {int: 'int', bool: 'bool', float: 'float', str: 'str', unicode: 'str'}

    names = _values.keys()
    names.sort()
    for name in names:
        value = _values[name]
        e = document.createElement('value')
        root.appendChild(e)
        e.setAttribute('name', name)
        e.setAttribute('type', valueNames[type(value)])
        valueElement = document.createTextNode(str(value))
        e.appendChild(valueElement)

    try:
        f = file(CONFIG_FILE, 'w')
    except IOError:
        pass
    else:
        document.writexml(f)
        f.close()
