# -*- coding: utf-8 -*-

## theater D-Bus plugin
## Copyright (C) 2009 Lucky <lucky1.data@gmail.com>
## Copyright (C) 2009 Philip Withnall <philip@tecnocode.co.uk>
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor,
## Boston, MA 02110-1301  USA.
##
## Sunday 13th May 2007: Bastien Nocera: Add exception clause.
## See license_change file for details.

import gettext
from gi.repository import GObject, Peas, theater # pylint: disable=no-name-in-module
import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop

gettext.textdomain ("theater")
_ = gettext.gettext

class DbusService (GObject.Object, Peas.Activatable):
    __gtype_name__ = 'DbusService'

    object = GObject.property (type = GObject.Object)

    def __init__ (self):
        GObject.Object.__init__ (self)

        self.root = None

    def do_activate (self):
        DBusGMainLoop (set_as_default = True)

        name = dbus.service.BusName ('org.mpris.MediaPlayer2.theater',
                                     bus = dbus.SessionBus ())
        self.root = Root (name, self.object)

    def do_deactivate (self):
        # Ensure we don't leak our paths on the bus
        self.root.disconnect ()

class Root (dbus.service.Object): # pylint: disable=R0904
    def __init__ (self, name, theater):
        dbus.service.Object.__init__ (self, name, '/org/mpris/MediaPlayer2')
        self.theater = theater

        self.null_metadata = {
            'year' : '', 'tracknumber' : '', 'location' : '',
            'title' : '', 'album' : '', 'time' : '', 'genre' : '',
            'artist' : ''
        }
        self.current_metadata = self.null_metadata.copy ()
        self.current_position = 0

        theater.connect ('metadata-updated', self.__do_update_metadata)
        theater.connect ('notify::playing', self.__do_notify_playing)
        theater.connect ('notify::seekable', self.__do_notify_seekable)
        theater.connect ('notify::current-mrl', self.__do_notify_current_mrl)
        theater.connect ('notify::current-time', self.__do_notify_current_time)

    def disconnect (self):
        self.theater.disconnect_by_func (self.__do_notify_current_time)
        self.theater.disconnect_by_func (self.__do_notify_current_mrl)
        self.theater.disconnect_by_func (self.__do_notify_seekable)
        self.theater.disconnect_by_func (self.__do_notify_playing)
        self.theater.disconnect_by_func (self.__do_update_metadata)

        self.__do_update_metadata (self.theater, '', '', '', 0)

        self.remove_from_connection (None, None)

    def __calculate_playback_status (self):
        if self.theater.is_playing ():
            return 'Playing'
        if self.theater.is_paused ():
            return 'Paused'
        return 'Stopped'

    def __calculate_metadata (self):
        metadata = {
            'mpris:trackid': dbus.String (self.theater.props.current_mrl,
                                          variant_level = 1),
            'mpris:length': dbus.Int64 (
                self.theater.props.stream_length * 1000,
                variant_level = 1),
        }

        if self.current_metadata['title'] != '':
            metadata['xesam:title'] = dbus.String (
                self.current_metadata['title'], variant_level = 1)

        if self.current_metadata['artist'] != '':
            metadata['xesam:artist'] = dbus.Array (
                [ self.current_metadata['artist'] ], variant_level = 1)

        if self.current_metadata['album'] != '':
            metadata['xesam:album'] = dbus.String (
                self.current_metadata['album'], variant_level = 1)

        if self.current_metadata['tracknumber'] != '':
            metadata['xesam:trackNumber'] = dbus.Int32 (
                self.current_metadata['tracknumber'], variant_level = 1)

        return metadata

    def __do_update_metadata (self, _, artist, # pylint: disable=R0913
                              title, album, num):
        self.current_metadata = self.null_metadata.copy ()
        if title:
            self.current_metadata['title'] = title
        if artist:
            self.current_metadata['artist'] = artist
        if album:
            self.current_metadata['album'] = album
        if num:
            self.current_metadata['tracknumber'] = num

        self.PropertiesChanged ('org.mpris.MediaPlayer2.Player',
                                { 'Metadata': self.__calculate_metadata () }, [])

    def __do_notify_playing (self, _, prop): # pylint: disable=W0613
        self.PropertiesChanged ('org.mpris.MediaPlayer2.Player',
                                { 'PlaybackStatus': self.__calculate_playback_status () }, [])

    def __do_notify_current_mrl (self, _, prop): # pylint: disable=W0613
        self.PropertiesChanged ('org.mpris.MediaPlayer2.Player', {
            'CanPlay': (self.theater.props.current_mrl is not None),
            'CanPause': (self.theater.props.current_mrl is not None),
            'CanSeek': (self.theater.props.current_mrl is not None and
                        self.theater.props.seekable),
            'CanGoNext': self.theater.can_seek_next (),
            'CanGoPrevious': self.theater.can_seek_previous (),
        }, [])

    def __do_notify_seekable (self, _, prop): # pylint: disable=W0613
        self.PropertiesChanged ('org.mpris.MediaPlayer2.Player', {
            'CanSeek': (self.theater.props.current_mrl is not None and
                        self.theater.props.seekable),
        }, [])

    def __do_notify_current_time (self, theater, _):
        # Only notify of seeks if we've skipped more than 3 seconds
        if abs (theater.props.current_time - self.current_position) > 3:
            self.Seeked (theater.props.current_time * 1000)

        self.current_position = theater.props.current_time

    # org.freedesktop.DBus.Properties interface
    @dbus.service.method (dbus_interface = dbus.PROPERTIES_IFACE,
                          in_signature = 'ss', # pylint: disable=C0103
                          out_signature = 'v')
    def Get (self, interface_name, property_name): # pylint: disable=C0103
        return self.GetAll (interface_name)[property_name]

    @dbus.service.method (dbus_interface = dbus.PROPERTIES_IFACE,
                          in_signature = 's', # pylint: disable=C0103
                          out_signature = 'a{sv}')
    def GetAll (self, interface_name): # pylint: disable=C0103
        if interface_name == 'org.mpris.MediaPlayer2':
            return {
                'CanQuit': True,
                'CanRaise': True,
                'HasTrackList': False,
                'Identity': 'Videos',
                'DesktopEntry': 'org.gnome.theater',
                'SupportedUriSchemes': self.theater.get_supported_uri_schemes (),
                'SupportedMimeTypes': self.theater.get_supported_content_types (),
            }

        if interface_name == 'org.mpris.MediaPlayer2.Player':
            # Loop status (we don't support Track)
            if self.theater.remote_get_setting (theater.RemoteSetting.REPEAT):
                loop_status = 'Playlist'
            else:
                loop_status = 'None'

            return {
                'PlaybackStatus': self.__calculate_playback_status (),
                'LoopStatus': loop_status, # TODO: Notifications
                'Rate': 1.0,
                'MinimumRate': 1.0,
                'MaximumRate': 1.0,
                'Metadata': self.__calculate_metadata (),
                'Volume': self.theater.get_volume (), # TODO: Notifications
                'Position': dbus.Int64(self.theater.props.current_time * 1000),
                'CanGoNext': self.theater.can_seek_next (),
                'CanGoPrevious': self.theater.can_seek_previous (),
                'CanPlay': (self.theater.props.current_mrl is not None),
                'CanPause': (self.theater.props.current_mrl is not None),
                'CanSeek': (self.theater.props.current_mrl is not None and
                            self.theater.props.seekable),
                'CanControl': True,
            }

        raise dbus.exceptions.DBusException (
            'org.mpris.MediaPlayer2.UnknownInterface',
            _('The MediaPlayer2 object does not implement the ‘%s’ interface')
            % interface_name)

    @dbus.service.method (dbus_interface = dbus.PROPERTIES_IFACE,
                          in_signature = 'ssv') # pylint: disable=C0103
    def Set (self, interface_name, property_name, # pylint: disable=C0103
             new_value):
        if interface_name == 'org.mpris.MediaPlayer2':
            raise dbus.exceptions.DBusException (
                'org.mpris.MediaPlayer2.ReadOnlyProperty',
                _('The property ‘%s’ is not writeable.'))

        if interface_name == 'org.mpris.MediaPlayer2.Player':
            if property_name == 'LoopStatus':
                self.theater.remote_set_setting (
                    theater.RemoteSetting.REPEAT, (new_value == 'Playlist'))
            elif property_name == 'Rate':
                # Ignore, since we don't support setting the rate
                pass
            elif property_name == 'Volume':
                self.theater.set_volume (new_value)

            raise dbus.exceptions.DBusException (
                'org.mpris.MediaPlayer2.ReadOnlyProperty',
                _('Unknown property ‘%s’ requested of a MediaPlayer 2 object')
                % interface_name)

        raise dbus.exceptions.DBusException (
            'org.mpris.MediaPlayer2.UnknownInterface',
            _('The MediaPlayer2 object does not implement the ‘%s’ interface')
            % interface_name)

    @dbus.service.signal (dbus_interface = dbus.PROPERTIES_IFACE,
                          signature = 'sa{sv}as') # pylint: disable=C0103
    def PropertiesChanged (self, interface_name,  # pylint: disable=C0103
                           changed_properties, invalidated_properties):
        pass

    # org.mpris.MediaPlayer2 interface
    @dbus.service.method (dbus_interface = 'org.mpris.MediaPlayer2',
                          in_signature = '', # pylint: disable=C0103
                          out_signature = '')
    def Raise (self): # pylint: disable=C0103
        main_window = self.theater.get_main_window ()
        main_window.present ()

    @dbus.service.method (dbus_interface = 'org.mpris.MediaPlayer2',
                          in_signature = '', # pylint: disable=C0103
                          out_signature = '')
    def Quit (self): # pylint: disable=C0103
        self.theater.exit ()

    # org.mpris.MediaPlayer2.Player interface
    @dbus.service.method (dbus_interface = 'org.mpris.MediaPlayer2.Player',
                          in_signature = '', # pylint: disable=C0103
                          out_signature = '')
    def Next (self): # pylint: disable=C0103
        self.theater.seek_next ()

    @dbus.service.method (dbus_interface = 'org.mpris.MediaPlayer2.Player',
                          in_signature = '', # pylint: disable=C0103
                          out_signature = '')
    def Previous (self): # pylint: disable=C0103
        self.theater.seek_previous ()

    @dbus.service.method (dbus_interface = 'org.mpris.MediaPlayer2.Player',
                          in_signature = '', # pylint: disable=C0103
                          out_signature = '')
    def Pause (self): # pylint: disable=C0103
        self.theater.pause ()

    @dbus.service.method (dbus_interface = 'org.mpris.MediaPlayer2.Player',
                          in_signature = '', # pylint: disable=C0103
                          out_signature = '')
    def PlayPause (self): # pylint: disable=C0103
        self.theater.play_pause ()

    @dbus.service.method (dbus_interface = 'org.mpris.MediaPlayer2.Player',
                          in_signature = '', # pylint: disable=C0103
                          out_signature = '')
    def Stop (self): # pylint: disable=C0103
        self.theater.stop ()

    @dbus.service.method (dbus_interface = 'org.mpris.MediaPlayer2.Player',
                          in_signature = '', # pylint: disable=C0103
                          out_signature = '')
    def Play (self): # pylint: disable=C0103
        # If playing or no track loaded: do nothing,
        # else: start playing.
        if self.theater.is_playing () or self.theater.props.current_mrl is None:
            return

        self.theater.play ()

    @dbus.service.method (dbus_interface = 'org.mpris.MediaPlayer2.Player',
                          in_signature = 'x', # pylint: disable=C0103
                          out_signature = '')
    def Seek (self, offset): # pylint: disable=C0103
        self.theater.seek_relative (offset / 1000, False)

    @dbus.service.method (dbus_interface = 'org.mpris.MediaPlayer2.Player',
                          in_signature = 'ox', # pylint: disable=C0103
                          out_signature = '')
    def SetPosition (self, _, position): # pylint: disable=C0103
        position = position / 1000

        # Bail if the position is not in the permitted range
        if position < 0 or position > self.theater.props.stream_length:
            return

        self.theater.seek_time (position, False)

    @dbus.service.method (dbus_interface = 'org.mpris.MediaPlayer2.Player',
                          in_signature = 's', # pylint: disable=C0103
                          out_signature = '')
    def OpenUri (self, uri): # pylint: disable=C0103
        self.theater.add_to_playlist_and_play (uri)

    @dbus.service.signal (dbus_interface = 'org.mpris.MediaPlayer2.Player',
                          signature = 'x') # pylint: disable=C0103
    def Seeked (self, position): # pylint: disable=C0103
        pass
