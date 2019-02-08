#
#   gui.py
#
#   This file is part of Samuel   
#
#   Samuel is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Samuel is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Samuel.  If not, see <http://www.gnu.org/licenses/>.
#

from gi.repository import Gtk
from gi.repository import Gdk
from gi.repository import GLib
from gi.repository import Pango
from gi.repository import GdkPixbuf
import cairo
import sys
import os
if os.path.abspath(os.path.dirname(__file__)).startswith(sys.prefix):
    from samuel import engine
else:
    import engine
from .constants import *

class Gui:    

    def build_gui(self):                        

        # Create Main Window
        self.window = Gtk.Window(Gtk.WindowType.TOPLEVEL)
        #self.window.set_resizable(False)        
        self.window.set_title(NAME + " " + VERSION)
        self.window.set_default_size(550, 550) # startup size        
        self.window.set_size_request(400, 400) # minimum size

        # Set a handler for delete_event that immediately
        # exits GTK.
        self.window.connect("delete_event", self.game.delete_event)
        self.window.connect("key_press_event", self.game.key_press_event)
        self.window.connect("configure_event", self.configure_event)        
   	
        main_vbox = Gtk.VBox(False, 0)                
        self.window.add(main_vbox)
        main_vbox.show()
        
        # 1 eventbox per board square
        self.eb = [ \
            [Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox()], \
            [Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox()], \
            [Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox()], \
            [Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox()], \
            [Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox()], \
            [Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox()], \
            [Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox()], \
            [Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox(), Gtk.EventBox()], \
            ]     

        # menu
        # Create a UIManager instance
        uimanager = Gtk.UIManager()

        # Add the accelerator group to the toplevel window
        accelgroup = uimanager.get_accel_group()
        self.window.add_accel_group(accelgroup)

        # Create ActionGroups

        # main action group
        actiongroup = Gtk.ActionGroup('UIManagerAG')        
        self.actiongroup = actiongroup

        # action group for setting search depth level        
        search_depth_actiongroup = Gtk.ActionGroup('AGSearchDepth')              
        search_depth_actiongroup.add_radio_actions([('Beginner', None, '_Beginner', None, None, 0),
                                       ('Advanced', None, '_Advanced', None, None, 1),
                                       ('Expert', None, '_Expert', None, None, 2),
                                       ('Custom', None, '_User-defined', None, None, 3), 
                                       ], 0, self.game.set_level)        

        # action group for showing/hiding panel        
        panel_action_group = Gtk.ActionGroup('AGPanel')
        panel_action_group.add_toggle_actions([('showpanel', None, '_Information Panel', None, None,
                                               self.set_info_panel)])
        panel_action_group.add_toggle_actions([('statusbar', None, '_Status Bar', None, None,
                                               self.set_status_bar)])
        self.panel_action_group = panel_action_group                                        
        
        # Computer Player
        computer_player_action_group = Gtk.ActionGroup('ComputerPlayer')
        computer_player_action_group.add_radio_actions([('ComputerPlaysWhite', None, '_Computer Plays White', None, None, 0),
                                              ('ComputerPlaysRed', None, '_Computer Plays Red', None, None, 1),
                                              ('ComputerPlaysWhiteAndRed', None, '_Computer Plays White and Red', None, None, 2),
                                              ('ComputerOff', None, '_Computer Off', None, None, 3),  
                                        ], 0, self.game.set_computer_player) 
        self.computer_player_action_group = computer_player_action_group        

        # Create a ToggleAction for flipping the board        
        flip_the_board_action_group = Gtk.ActionGroup('FlipTheBoard')
        flip_the_board_action_group.add_toggle_actions([('FlipTheBoard', None, '_Flip the Board', None,
                                         'Flip the Board', self.game.flip_the_board)])
        self.flip_the_board_action_group = flip_the_board_action_group

        # Create actions
        actiongroup.add_actions([('Quit', Gtk.STOCK_QUIT, '_Quit', None, 'Quit the Program', self.game.quit_game),
                                 ('NewGame', Gtk.STOCK_NEW, '_New Game', None, 'New Game', self.game.new_game),
                                 ('LoadGame', Gtk.STOCK_OPEN, '_Load Game', None, 'Load Game', self.game.load_game),
                                 ('SaveGame', Gtk.STOCK_SAVE, '_Save Game', None, 'Save Game', self.game.save_game),
                                 ('MoveNow', None, '_Move Now (m)', None, 'Move Now', self.game.move_now),
                                 ('Game', None, '_Game'),
                                 ('PositionEdit', None, '_Position Edit', None, 'Position Edit', \
                                     self.board.position_edit_init),
                                 ('CopyFenToCB', None, '_Copy FEN to clipboard', None, 'Copy FEN to clipboard', \
                                     self.game.copy_FEN_to_clipboard),
                                 ('PasteFenFromCB', None, '_Paste FEN from clipboard', None, 'Paste FEN from clipboard', \
                                     self.game.paste_FEN_from_clipboard),       
                                 ('CopyPDNToCB', Gtk.STOCK_COPY, '_Copy PDN to clipboard', None, 'Copy PDN to clipboard', \
                                     self.game.copy_PDN_to_clipboard),
                                 ('PastePDNFromCB', Gtk.STOCK_PASTE, '_Paste PDN from clipboard', None, 'Paste PDN from clipboard', \
                                     self.game.paste_PDN_from_clipboard),                      
                                 ('Edit', None, '_Edit'),
                                 ('SetCustomLevelDepth', None, '_Set User-defined Level', None, 'Set Custom Level Depth', \
                                    self.game.set_custom_search_depth),                                 
                                 ('Level', None, '_Level'), 
                                 ('Options', None, '_Options'),                                                    
                                 ('About', Gtk.STOCK_ABOUT, '_About', None, 'Show About Box', self.about_box),
                                 ('samhelp', Gtk.STOCK_HELP, '_Help (online)', None, 'Samuel Help (Online)', \
                                    self.game.open_help),
                                 ('Help', None, '_Help'),
                                ])        
   
        actiongroup.get_action('Quit').set_property('short-label', '_Quit')
        actiongroup.get_action('MoveNow').set_sensitive(False)                                                    

        # Add the actiongroups to the uimanager
        uimanager.insert_action_group(actiongroup, 0)
        uimanager.insert_action_group(search_depth_actiongroup, 1)        
        uimanager.insert_action_group(panel_action_group, 2)
        uimanager.insert_action_group(computer_player_action_group, 3)        
        uimanager.insert_action_group(flip_the_board_action_group, 4)             

        # Action groups that need settings to be saved/restored on program exit/startup
        self.save_action_groups = [search_depth_actiongroup, panel_action_group, flip_the_board_action_group, \
                                    computer_player_action_group]  

        ui = '''<ui>
        <menubar name="MenuBar">
            <menu action="Game">
                <menuitem action="NewGame"/> 
                <separator/>   
                <menuitem action="LoadGame"/> 
                <menuitem action="SaveGame"/> 
                <separator/>  
                <menuitem action="MoveNow"/> 
                <separator/>
                <menuitem action="Quit"/>                        
            </menu>
            <menu action="Edit">
                <menuitem action="PositionEdit"/>
                <separator/> 
                <menuitem action="CopyFenToCB"/>
                <menuitem action="PasteFenFromCB"/> 
                <separator/> 
                <menuitem action="CopyPDNToCB"/>
                <menuitem action="PastePDNFromCB"/> 
            </menu>
            <menu action="Level">
                <menuitem action="Beginner"/>
                <menuitem action="Advanced"/>            
                <menuitem action="Expert"/> 
                <menuitem action="Custom"/>
                <separator/>
                <menuitem action="SetCustomLevelDepth"/>
                <separator/>            
            </menu>
            <menu action="Options">                             
                <menuitem action="ComputerPlaysWhite"/>
                <menuitem action="ComputerPlaysRed"/>
                <menuitem action="ComputerPlaysWhiteAndRed"/>
                <menuitem action="ComputerOff"/> 
                <separator/>                
                <menuitem action="showpanel"/> 
                <menuitem action="statusbar"/>                                             
                <separator/>             
                <menuitem action="FlipTheBoard"/>                    
            </menu>
            <menu action="Help">
                <menuitem action="samhelp"/>
                <separator/>
                <menuitem action="About"/> 
            </menu>
        </menubar>
        </ui>'''

        # Add a UI description
        uimanager.add_ui_from_string(ui)

        # Create a MenuBar
        menubar = uimanager.get_widget('/MenuBar')                
        main_vbox.pack_start(menubar, False, True, 0)        
        
        self.load_images()

        # Create a 8x8 table
        self.table = Gtk.Table(8, 8, True)
        self.table.set_border_width(25)
    
        aspect_frame = Gtk.AspectFrame(label=None, xalign=0.5, yalign=0.5, ratio=1.0, obey_child=False)
        aspect_frame.add(self.table)

        eb = Gtk.EventBox()
        #eb.add(self.table) 
        eb.add(aspect_frame) 
        eb.show()
        eb.modify_bg(Gtk.StateType.NORMAL, Gdk.color_parse("darkslategrey"))                 
        main_vbox.pack_start(eb, True, True, 0) 

        bot_hbox = Gtk.HBox(False, 0) 
        main_vbox.pack_start(bot_hbox, False, True, 7)                        
        bot_hbox.show()  

        vbox = Gtk.VBox(False, 0) 
        bot_hbox.pack_end(vbox, False, False, 5) 
        hbox1 = Gtk.HBox(False, 0) 
        vbox.pack_start(hbox1, True, False, 0) 
        hbox2 = Gtk.HBox(False, 0) 
        vbox.pack_start(hbox2, True, False, 7) 
        
        frame = Gtk.Frame()                              
        frame.set_shadow_type(Gtk.ShadowType.IN)       
        frame.show()        

        vp = Gtk.Viewport()
        vp.add(frame)
        vp.show()
        vp.modify_bg(Gtk.StateType.NORMAL, Gdk.color_parse('#EDECEB'))
       
        bot_hbox.pack_end(vp, True, True, 7)        

        self.bot_hbox = bot_hbox              

        self.infolabel = Gtk.Label()
        self.infolabel.modify_font(Pango.FontDescription("monospace 8"))                
        
        frame.add(self.infolabel)
        self.infolabel.show()        

        # get positions loaded msg  
        msg = engine.rdisp()          
        self.infolabel.set_text(msg)              

        # go button
        self.go_button = Gtk.Button("Go")
        self.go_button.connect("clicked", self.game.callback, 'Go') 
        hbox1.pack_start(self.go_button, True, False, 0) 
        self.go_button.show()
        
        # retract button
        self.retract_button = Gtk.Button("Retract")
        self.retract_button.connect("clicked", self.game.callback, 'Retract') 
        hbox1.pack_start(self.retract_button, True, False, 0) 
        self.retract_button.show()

        # rewind to start button
        self.rts_button = Gtk.Button("<|")
        self.rts_button.connect("clicked", self.game.callback, '<|') 
        hbox2.pack_start(self.rts_button, True, False, 0) 
        self.rts_button.show()

        # rewind 1 move buttton
        self.rom_button = Gtk.Button("<")
        self.rom_button.connect("clicked", self.game.callback, '<') 
        hbox2.pack_start(self.rom_button, True, False, 0) 
        self.rom_button.show()

        # forward 1 move buttton
        self.fom_button = Gtk.Button(">")
        self.fom_button.connect("clicked", self.game.callback, '>') 
        hbox2.pack_start(self.fom_button, True, False, 0) 
        self.fom_button.show()

        # forward to end of game button
        self.fteog_button = Gtk.Button("|>")
        self.fteog_button.connect("clicked", self.game.callback, '|>') 
        hbox2.pack_start(self.fteog_button, True, False, 0) 
        self.fteog_button.show()

        vbox.show()        
        hbox1.show()   	                           
        hbox2.show()        

        #
        # widgets for position edit
        #
        self.posedit_hbox = Gtk.HBox(False, 0) 
        main_vbox.pack_start(self.posedit_hbox, False, True, 7)         

        vbox = Gtk.VBox(False, 0) 
        self.posedit_hbox.pack_end(vbox, False, False, 5) 
        hbox1 = Gtk.HBox(False, 0) 
        vbox.pack_start(hbox1, True, False, 0) 
        label = Gtk.Label()
        label.set_text("Side to Move")
        label.show()
        vbox.pack_start(label, True, False, 7) 
        hbox2 = Gtk.HBox(False, 0) 
        vbox.pack_start(hbox2, True, False, 0) 
        
        frame = Gtk.Frame()                              
        frame.set_shadow_type(Gtk.ShadowType.IN)        
        frame.show()        

        vp = Gtk.Viewport()
        vp.add(frame)
        vp.show()
        vp.modify_bg(Gtk.StateType.NORMAL, Gdk.color_parse('#EDECEB'))
       
        self.posedit_hbox.pack_end(vp, True, True, 7)                  

        self.infolabel2 = Gtk.Label()
        self.infolabel2.modify_font(Pango.FontDescription("monospace 8"))                 
        
        frame.add(self.infolabel2)
        self.infolabel2.show()        
        
        self.cancel_button = Gtk.Button("Cancel")
        self.cancel_button.connect("clicked", self.game.callback, 'Cancel') 
        hbox1.pack_start(self.cancel_button, True, False, 0) 
        self.cancel_button.show()       

        button = Gtk.Button("OK")
        button.connect("clicked", self.game.callback, 'OK') 
        hbox1.pack_start(button, True, False, 0) 
        button.show()        

        self.radio_button_red = Gtk.RadioButton.new_with_label_from_widget(None, "Red")        
        hbox2.pack_start(self.radio_button_red, True, False, 0) 
        self.radio_button_red.show()

        self.radio_button_white = Gtk.RadioButton.new_with_label_from_widget(self.radio_button_red, "White")
        hbox2.pack_start(self.radio_button_white, True, False, 0) 
        self.radio_button_white.show()        

        vbox.show()        
        hbox1.show()   	                           
        hbox2.show()

        # status bar 
        self.status_bar = Gtk.Statusbar() 
        main_vbox.pack_start(self.status_bar, False, False, 0)        
        self.context_id = self.status_bar.get_context_id("samuel statusbar")        
        self.set_status_bar_msg("Red to Move")           

        self.window.show_all() 
        self.posedit_hbox.hide()        

        panel_action_group.get_action('showpanel').set_active(True) 
        panel_action_group.get_action('statusbar').set_active(True)       
         

    def set_refs(self, game, board):
        self.game = game
        self.board = board        


    def load_images(self):
        
        prefix = self.game.get_prefix()
        
        # create a cairo surface from each image 
        self.wchecksel = cairo.ImageSurface.create_from_png(os.path.join(prefix, "images/Wchecksel.png"))
        self.wcheck    = cairo.ImageSurface.create_from_png(os.path.join(prefix, "images/Wcheck.png"))
        self.rchecksel = cairo.ImageSurface.create_from_png(os.path.join(prefix, "images/Rchecksel.png"))
        self.rcheck    = cairo.ImageSurface.create_from_png(os.path.join(prefix, "images/Rcheck.png"))
        self.bsquare   = cairo.ImageSurface.create_from_png(os.path.join(prefix, "images/Bsquare.png"))
        self.wkingsel  = cairo.ImageSurface.create_from_png(os.path.join(prefix, "images/Wkingsel.png"))
        self.wking     = cairo.ImageSurface.create_from_png(os.path.join(prefix, "images/Wking.png"))
        self.rkingsel  = cairo.ImageSurface.create_from_png(os.path.join(prefix, "images/Rkingsel.png"))
        self.rking     = cairo.ImageSurface.create_from_png(os.path.join(prefix, "images/Rking.png"))
        self.wsquare   = cairo.ImageSurface.create_from_png(os.path.join(prefix, "images/Wsquare.png"))

        # Drag and Drop
        #self.wcheckdnd = cairo.ImageSurface.create_from_png(os.path.join(prefix, "images/WcheckDnD.png"))        
        self.wcheckdnd = GdkPixbuf.Pixbuf.new_from_file(os.path.join(prefix, "images/WcheckDnD.png"))
        self.rcheckdnd = GdkPixbuf.Pixbuf.new_from_file(os.path.join(prefix, "images/RcheckDnD.png"))
        self.wkingdnd = GdkPixbuf.Pixbuf.new_from_file(os.path.join(prefix, "images/WkingDnD.png"))
        self.rkingdnd = GdkPixbuf.Pixbuf.new_from_file(os.path.join(prefix, "images/RkingDnD.png"))

        self.board_squares = [
                    (37, 1, 0), (38, 3, 0), (39, 5, 0), (40, 7, 0),         
                    (32, 0, 1), (33, 2, 1), (34, 4, 1), (35, 6, 1),
                    (28, 1, 2), (29, 3, 2), (30, 5, 2), (31, 7, 2),        
                    (23, 0, 3), (24, 2, 3), (25, 4, 3), (26, 6, 3),
                    (19, 1, 4), (20, 3, 4), (21, 5, 4), (22, 7, 4),        
                    (14, 0, 5), (15, 2, 5), (16, 4, 5), (17, 6, 5),
                    (10, 1, 6), (11, 3, 6), (12, 5, 6), (13, 7, 6),        
                    (5, 0, 7), (6, 2, 7), (7, 4, 7), (8, 6, 7)]
 
    # about box
    def about_box(self, widget):
        about = Gtk.AboutDialog()        
        about.set_program_name(NAME)
        about.set_version(VERSION)
        about.set_copyright('Copyright \u00A9 2009,2019 John Cheetham')                             
        about.set_comments("Samuel is a draughts program for the Gnome desktop.\n \
It is based on the windows program guicheckers")
        about.set_authors(["John Cheetham"])
        about.set_website("http://www.johncheetham.com/projects/samuel/index.html")
        about.set_logo(GdkPixbuf.Pixbuf.new_from_file(os.path.join(self.game.prefix, "images/logo.png")))

        license = '''Samuel is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Samuel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Samuel.  If not, see <http://www.gnu.org/licenses/>.'''

        about.set_license(license)
        about.set_transient_for(self.window)
        about.run()
        about.destroy()


    def init_black_board_square(self, x, y):
        
        da = Gtk.DrawingArea()
        #da.connect('draw', self.da_draw_eventw, x, y)
        da.show()

        event_box = Gtk.EventBox()
        da.connect('draw', self.eb_draw_event, x, y)  
        event_box.add(da) 
        self.table.attach(event_box, x, x+1, y, y+1)
        event_box.show()    
        event_box.add_events(Gdk.EventMask.BUTTON_PRESS_MASK)
        data = (x, y)
        event_box.connect('button_press_event', self.game.square_clicked_CB, data)
        #image.show()
       
        self.eb[x][y] = event_box

        # connect signals for drag source
        event_box.connect("drag_begin", self.drag_begin, (x, y))
        event_box.connect("drag-data-get", self.sendCallback)
        event_box.connect("drag_end", self.drag_end)

        # connect signals for drag dest
        event_box.connect("drag-data-received", self.receiveCallback, (x, y))

   # set up drag and drop
   # see http://python-gtk-3-tutorial.readthedocs.org/en/latest/drag_and_drop.html
   # see https://wiki.gnome.org/GnomeLove/DragNDropTutorial    
    def init_all_dnd(self):      
        self.unset_all_dnd()      
        stm = self.game.get_side_to_move()        
        red_player, white_player = self.game.get_players()
        if stm == RED and red_player != HUMAN:   
            return
        if stm == WHITE and white_player != HUMAN:             
            return 
        
        # loop through all squares on the board setting the correct
        # drag src/dst for that square
        board_squares = self.board.get_board_squares()
        for sq in board_squares:
            gc_loc, x, y = sq            
            pce = self.board.get_piece_at_square(x, y)
            if (stm == RED and (pce == 1 or pce == 5)) or (stm == WHITE and (pce == 2 or pce == 6)):                
                self.eb[x][y].drag_source_set(Gdk.ModifierType.BUTTON1_MASK, [], Gdk.DragAction.COPY)           
                self.eb[x][y].drag_source_add_text_targets()            
            elif pce == 0:                
                self.eb[x][y].drag_dest_set(Gtk.DestDefaults.ALL, [], Gdk.DragAction.COPY)           
                self.eb[x][y].drag_dest_add_text_targets()           

    def unset_all_dnd(self):
        board_squares = self.board.get_board_squares()                
        for sq in board_squares:
            gc_loc, x, y = sq
            self.eb[x][y].drag_source_unset()
            self.eb[x][y].drag_dest_unset()

    def drag_begin(self, widget, drag_context, data):       
        self.dnd_data_received = False 
        # get x,y co-ords of source square
        x, y = data            

        # convert the x, y co-ords into the gui checkers representation
        gc_loc = self.board.get_gc_loc(x, y) 
        self.src = gc_loc
        # set the icon for the drag and drop to the piece that is being dragged    
        pce = self.board.get_piece_at_square(x, y)
        self.dnd_pce = pce  
       
        #   0 is unoccupied
        #   1 is a red piece   
        #   2 is a white piece
        #   5 is a red king
        #   6 is a white king
        if pce == 1:
            scaled_pixbuf = self.rcheckdnd.scale_simple(self.width, self.height, GdkPixbuf.InterpType.HYPER) 
        elif pce == 2:
            scaled_pixbuf = self.wcheckdnd.scale_simple(self.width, self.height, GdkPixbuf.InterpType.HYPER)
        elif pce == 5:
            scaled_pixbuf = self.rkingdnd.scale_simple(self.width, self.height, GdkPixbuf.InterpType.HYPER)
        elif pce == 6:
            scaled_pixbuf = self.wkingdnd.scale_simple(self.width, self.height, GdkPixbuf.InterpType.HYPER)
        else:
            return
        
        hot_x = scaled_pixbuf.get_width() / 2
        hot_y = scaled_pixbuf.get_height() / 2      
        Gtk.drag_set_icon_pixbuf(drag_context, scaled_pixbuf, hot_x, hot_y)

        # Set piece at source square as unoccupied (0)
        self.board.set_piece_at_square(gc_loc, 0)
        self.eb[x][y].queue_draw()

    # if drag and drop failed then reinstate the piece where it
    # was dragged from
    def drag_end(self, widget, drag_context):        
        # if receiveCallback function not entered then restore board
        # to before the drag started
        if not self.dnd_data_received:            
            # set piece back at source square            
            self.board.set_piece_at_square(self.src, self.dnd_pce)
            self.board.display_board()          
            return        

    def sendCallback(self, widget, context, selection, targetType, eventTime):
        selection.set_text("samuel", -1)

    def receiveCallback(self, widget, context, x, y, selection, targetType,
                        time, data):        
        self.dnd_data_received = True        

        if selection.get_text() != "samuel":
            print("invalid selection data. ignored")
            return False

        x, y = data
        # convert the x, y co-ords into the gui checkers representation
        gc_loc = self.board.get_gc_loc(x, y) 
        self.dst = gc_loc
        board_position = engine.hmove(self.src, self.dst)
        self.board.set_board_position(board_position)       
        self.board.set_board_status()
      
        # fixme
        if self.game.legalmove == 0: 
            # illegal move so set piece back at source square
            zx, zy = self.board.get_x_y(self.src)
            pce = self.board.get_piece_at_square(zx, zy)
            self.board.set_piece_at_square(self.src, pce)
            self.board.display_board()
            return False

        # Set piece at dest square as dragged piece
        pce = self.board.get_piece_at_square(x, y) 
        self.board.set_piece_at_square(gc_loc, pce)              

        # if move was a jump then set the square jumped over to empty
        diff = self.dst - self.src
        if diff < 0: diff = -diff
        hdiff = diff / 2
       
        # 8 or 10 is a jump
        # 4 or 5 is normal move 
        if diff > 5:
            if self.dst > self.src:                
                gc_loc = self.src + hdiff 
            else:
                gc_loc = self.src - hdiff               
            self.board.set_piece_at_square(gc_loc, 0)
            zx,zy = self.board.get_x_y(gc_loc)           
            self.eb[zx][zy].queue_draw()           

        Gtk.drag_finish(context, True, True, time)

        # fixme
        if self.game.legalmove == 2000:
            # double jump not yet completed - don't call computer move
            self.init_all_dnd()
            return False

        stm = self.game.get_side_to_move()        
        red_player, white_player = self.game.get_players()        

        self.init_all_dnd()

        # If human is playing both sides (i.e. computer is off) then return
        # now without calling the engine
        if stm == RED and red_player != COMPUTER:                    
            return False

        if stm == WHITE and white_player != COMPUTER:                    
            return False 

        # kick off computer move
        GLib.timeout_add(1000, self.game.comp_move)
        self.board.display_board()
        return False

    def draw_board(self):       
        self.table.queue_draw()

    def eb_draw_event(self, eb, cr, x, y):
        allocation = eb.get_allocation()
        width, height = allocation.width, allocation.height
        self.width = width
        self.height = height
        cr.scale(width / 64.00, height / 64.00) 
        pce = self.board.get_piece_at_square(x, y)
        gc_loc = self.board.get_gc_loc(x, y)
        if self.game.get_src() == gc_loc:
            selected = True 
        else:
            selected = False
        
        #   0 is unoccupied
        #   1 is a red piece   
        #   2 is a white piece
        #   5 is a red king
        #   6 is a white king 
        if pce == 0:            
            cr.set_source_surface(self.bsquare, 0.0, 0.0)       
        elif pce == 1:
            if selected:
                cr.set_source_surface(self.rchecksel, 0.0, 0.0)
            else: 
                cr.set_source_surface(self.rcheck, 0.0, 0.0)
        elif pce == 2:
            if selected:
                cr.set_source_surface(self.wchecksel, 0.0, 0.0)
            else: 
                cr.set_source_surface(self.wcheck, 0.0, 0.0)
        elif pce == 5:
            if selected:
                cr.set_source_surface(self.rkingsel, 0.0, 0.0)
            else:
                cr.set_source_surface(self.rking, 0.0, 0.0)
        elif pce == 6:
            if selected:
                cr.set_source_surface(self.wkingsel, 0.0, 0.0)
            else:
                cr.set_source_surface(self.wking, 0.0, 0.0)
        else:
            cr.set_source_surface(self.bsquare, 0.0, 0.0)
        cr.paint() 

    def init_white_board_square(self, x, y):
        da = Gtk.DrawingArea()
        da.connect('draw', self.da_draw_eventw, x, y)
        da.show()
        self.table.attach(da, x, x+1, y, y+1, Gtk.AttachOptions.FILL, Gtk.AttachOptions.FILL)

    def da_draw_eventw(self, da, cr, x, y):      
        allocation = da.get_allocation()
        width, height = allocation.width, allocation.height
        cr.scale(width / 64.00, height / 64.00) 
        cr.set_source_surface(self.wsquare, 0.0, 0.0)       
        cr.paint()

    # return the side to move after a position edit as set in the radio button
    def get_side_to_move(self):
        if self.radio_button_red.get_active():                    
            return RED
        else:
            return WHITE

    def set_panel_font_size(self, font_size):
        fs = str(font_size) 
        self.infolabel.modify_font(Pango.FontDescription("monospace " + fs))
        self.infolabel2.modify_font(Pango.FontDescription("monospace " + fs))
        # Make the bottom panels display correctly after a resize
        if self.bot_hbox.get_visible():                
            self.bot_hbox.hide()                    
            self.bot_hbox.show()
        if self.posedit_hbox.get_visible():       
            self.posedit_hbox.hide()                    
            self.posedit_hbox.show()         

    def init_posedit_panel(self, text):
        self.infolabel2.set_text(text) 
        # save status of infopanel(visible or not) so it can be restored at end of position edit        
        if self.bot_hbox.get_visible():
            self.infopanel_visible = True
        else:
            self.infopanel_visible = False

        # Hide info panel - we'll replace it with position edit panel        
        self.bot_hbox.hide()       

        if self.game.get_side_to_move() == RED:
            self.radio_button_red.set_active(True)
        else:
            self.radio_button_white.set_active(True)

        # Make position edit panel visible
        self.posedit_hbox.show()

    def hide_posedit_panel(self):
        self.posedit_hbox.hide()
                
        # if infopanel was visible prior to position edit then show it        
        if self.infopanel_visible:                    
            self.bot_hbox.show()            

    def enable_move_now(self):
        self.actiongroup.get_action('MoveNow').set_sensitive(True)

    def disable_move_now(self):        
        self.actiongroup.get_action('MoveNow').set_sensitive(False)

    def set_panel_text(self, text):
        self.infolabel.set_text(text) 

    def get_panel_text(self):
        return self.infolabel.get_text() 

    def append_panel_text(self, text):
        t = self.infolabel.get_text()
        if len(t) != 0:
            t = t + "\n"
        t = t + text       
        self.infolabel.set_text(t)

    # Show/Hide Information Panel
    def set_info_panel(self, w):                  
        if w.get_active():        
            self.bot_hbox.show()            
        else:        
            self.bot_hbox.hide()
    

    # Show/Hide Status Bar
    def set_status_bar(self, w):                  
        if w.get_active():
            self.status_bar.show()            
        else:
            self.status_bar.hide()
    
    #
    # Activate a menu option. It is called at startup to restore menu settings
    # to the values from the previous game.    
    #    
    def activate(self, tup):              
        action_type = tup[0]                
        if action_type == 'radioaction':            
            (at, ag_name, a_name) = tup
            for ag in self.save_action_groups:            
                if ag.get_name() == ag_name:                
                    action = ag.get_action(a_name)                    
                    try:                        
                        action.activate()
                    except Exception as e:
                        print("unable to set ",ag_name)                    

        elif action_type == 'toggleaction':            
            (at, ag_name, a_name, active) = tup
            for ag in self.save_action_groups:            
                if ag.get_name() == ag_name:                
                    action = ag.get_action(a_name)
                    action.set_active(active)                       
            
    # get the radio action settings set by the user
    def get_action_settings(self):     

        lst = []        

        for ag in self.save_action_groups:            
            ag_name = ag.get_name()
            action_list = ag.list_actions()
            
            for a in action_list:                
                # RadioActions - save the active action
                if isinstance(a, Gtk.RadioAction):                    
                    if a.get_active():                        
                        a_name = a.get_name()                    
                        tup = ('radioaction', ag_name, a_name)
                        lst.append(tup)
                # Toggle Actions - save the on/off setting
                elif isinstance(a, Gtk.ToggleAction):                     
                    a_name = a.get_name()                    
                    tup = ('toggleaction', ag_name, a_name, a.get_active())
                    lst.append(tup)                    
        
        return lst

    def get_screen_size(self):
        screen = self.window.get_screen()        
        return (screen.get_width(), screen.get_height())     

    def get_main_window_size(self):        
        #return self.window.get_size()
        return (self.win_width, self.win_height)
    
    def set_window_size(self, width, height):
        self.window.resize(width, height)

    # process window size change
    def configure_event(self, widget, event):        
        self.win_width = event.width
        self.win_height = event.height

    # disable some functionality if computer is thinking    
    def disable_menu_items(self):        
        self.retract_button.set_sensitive(False)        
        self.rts_button.set_sensitive(False)
        self.rom_button.set_sensitive(False)
        self.fom_button.set_sensitive(False)
        self.fteog_button.set_sensitive(False)
        self.actiongroup.get_action('NewGame').set_sensitive(False)
        self.actiongroup.get_action('LoadGame').set_sensitive(False)
        self.actiongroup.get_action('SaveGame').set_sensitive(False)    
        self.actiongroup.get_action('Edit').set_sensitive(False)
        self.actiongroup.get_action('Level').set_sensitive(False)    
        self.actiongroup.get_action('MoveNow').set_sensitive(True)        
        self.computer_player_action_group.set_sensitive(False)        
        self.flip_the_board_action_group.get_action('FlipTheBoard').set_sensitive(False)

    # enable some functionality after computers move or after in position edit    
    def enable_menu_items(self):        
        self.retract_button.set_sensitive(True)        
        self.rts_button.set_sensitive(True)
        self.rom_button.set_sensitive(True)
        self.fom_button.set_sensitive(True)
        self.fteog_button.set_sensitive(True)
        self.actiongroup.get_action('NewGame').set_sensitive(True)
        self.actiongroup.get_action('LoadGame').set_sensitive(True)
        self.actiongroup.get_action('SaveGame').set_sensitive(True)    
        self.actiongroup.get_action('Edit').set_sensitive(True)
        self.actiongroup.get_action('Level').set_sensitive(True)    
        self.actiongroup.get_action('MoveNow').set_sensitive(False)        
        self.computer_player_action_group.set_sensitive(True)
        self.flip_the_board_action_group.get_action('FlipTheBoard').set_sensitive(True)
       

    # disable some functionality if in position edit    
    def disable_posedit_menu_items(self):        
        self.actiongroup.get_action('NewGame').set_sensitive(False)
        self.actiongroup.get_action('LoadGame').set_sensitive(False)
        self.actiongroup.get_action('SaveGame').set_sensitive(False)    
        self.actiongroup.get_action('Edit').set_sensitive(False)
        self.actiongroup.get_action('Level').set_sensitive(False)    
        self.actiongroup.get_action('MoveNow').set_sensitive(False)         
        self.panel_action_group.set_sensitive(False)        
        self.computer_player_action_group.set_sensitive(False)        
        self.flip_the_board_action_group.get_action('FlipTheBoard').set_sensitive(False)


    # enable some functionality after computers move or after in position edit    
    def enable_posedit_menu_items(self):        
        self.actiongroup.get_action('NewGame').set_sensitive(True)
        self.actiongroup.get_action('LoadGame').set_sensitive(True)
        self.actiongroup.get_action('SaveGame').set_sensitive(True)    
        self.actiongroup.get_action('Edit').set_sensitive(True)
        self.actiongroup.get_action('Level').set_sensitive(True)    
        self.actiongroup.get_action('MoveNow').set_sensitive(True)        
        self.panel_action_group.set_sensitive(True)      
        self.computer_player_action_group.set_sensitive(True)
        self.flip_the_board_action_group.get_action('FlipTheBoard').set_sensitive(True)


    def set_status_bar_msg(self, msg):
        self.status_bar.push(self.context_id, msg)


    def get_window(self):
        return self.window
