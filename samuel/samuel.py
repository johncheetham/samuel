#
#   Samuel 0.1.8    October 2009
#
#   Copyright (C) 2009 John Cheetham    
#   
#   web   : http://www.johncheetham.com/projects/samuel
#   email : developer@johncheetham.com
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

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
from gi.repository import Gdk
from gi.repository import GLib

import sys, time, _thread, traceback
import engine
from gi.repository import GObject
import os
import errno
import pickle
import webbrowser

from . import gui, board
from .constants import *


class Game:    

    def __init__(self):
        
        # initialise variables
        self.src = 0
        self.dst = 0
        self.thinking = False
        self.gameover = 0                
        self.side_to_move = RED        
        #self.computer_colour = WHITE
        self.red_player = HUMAN
        self.white_player = COMPUTER
        self.custom_search_depth = 2 
        self.custom_time_limit = 2
        self.level = 0                               

        # set paths to images. opening book etc
        (opening_book_path, end_game_2pc_path, end_game_3pc_path, end_game_4pc_path) = self.set_data_paths()        
        
        # initialise the engine
        # Pass in the paths of the opening book and endgame databases to load
        board_position = engine.init(opening_book_path, end_game_2pc_path, end_game_3pc_path, end_game_4pc_path)        

        # instantiate board, gui, classes        
        self.board = board.Board(board_position)
        self.gui = gui.Gui()        
        self.board.set_refs(self, self.gui)
        self.gui.set_refs(self, self.board)              

        # build gui, board
        self.gui.build_gui()
        self.board.build_board()

        # restore users settings to values from previous game
        self.restore_settings()    

        # set status bar msg
        self.gui.set_status_bar_msg(self.get_side_to_move_msg())
        self.gui.init_all_dnd()
 

    def set_data_paths(self):

        print("sys.prefix=", sys.prefix)
        print("sys.exec_prefix=", sys.exec_prefix)
        # Find the absolute path that this python program is running in 
        progpath = os.path.abspath(os.path.dirname(__file__))        
        print("progpath=", progpath)

        # work out if we are running from an installed version
        # or from the source directory
        if progpath.startswith(sys.prefix):
            print("we are installed")
            self.prefix = os.path.join (sys.prefix, "share/samuel")
            
            if os.path.isdir(self.prefix):
                print("images/data path=", self.prefix)
            else:
                print("setting images/data path")

                for dir in ("share", "games", "share/games",
                    "local/share", "local/games", "local/share/games"):
                    self.prefix = os.path.join (sys.prefix, dir, "samuel")
                    if os.path.isdir(self.prefix):
                        print("found images/data path=", self.prefix)                        
                        break
                else:
                    raise Exception("can't find data directory")
       
        else:
            print("we are NOT installed")
            # get data files (images, opening book, endgame databases) from same directory as this program 
            self.prefix = os.path.abspath(os.path.dirname(__file__))
            self.prefix = os.path.dirname(self.prefix)            
            print("using images/data path=", self.prefix) 


        # set up samuel directory under home directory
        self.sampath = os.path.expanduser("~") + "/.samuel"        
        if not os.path.exists(self.sampath):
            try:
                os.makedirs(self.sampath)
            except OSError as exc:
                #if exc.errno == errno.EEXIST:
                #    pass
                #else:
                raise               

        opening_book_path = os.path.expanduser("~") + "/.samuel/opening.gbk"
        if not os.path.exists(opening_book_path):
            opening_book_path = os.path.join(self.prefix, "data/opening.gbk")

        end_game_2pc_path = os.path.join(self.prefix, "data/2pc.cdb")
        end_game_3pc_path = os.path.join(self.prefix, "data/3pc.cdb")
        end_game_4pc_path = os.path.join(self.prefix, "data/4pc.cdb")

        return (opening_book_path, end_game_2pc_path, end_game_3pc_path, end_game_4pc_path) 

    def square_clicked_CB(self, widget, event, data):               
        #self.gui.draw_board()

        # if we are in position edit mode then pass all clicks on squares to the
        # edit routine        
        if self.board.get_pos_edit():
            self.board.position_edit(event, data)
            return
        
        # don't allow moves if game is over
        if self.gameover != 0:            
            return        

        if self.side_to_move == RED and self.red_player != HUMAN:            
            return

        if self.side_to_move == WHITE and self.white_player != HUMAN:            
            return             

        self.square_clicked(data)

    def square_clicked(self, data):

        # get x,y co-ords of square clicked on
        x, y = data        
        
        # convert the x, y co-ords into the gui checkers representation
        gc_loc = self.board.get_gc_loc(x, y)         

        # if the square clicked on is a valid source square
        # then set this square as the source square
        if self.board.valid_source_square(gc_loc, self.side_to_move):         
            self.src = gc_loc            
            # call display board to set the square clicked on as highlighted
            self.board.display_board()
            return

        # if the square clicked on is a valid destination square (i.e. unoccupied)
        # then set this square as the destination square. The engine will check
        # if the move is legal
        if self.board.valid_destination_square(gc_loc):        
            self.dst = gc_loc
            
            # call the engine to update with the Human move source square (src)
            # to destination square (dst)            
            board_position = engine.hmove(self.src, self.dst)
            self.board.set_board_position(board_position)                      
            self.board.display_board()
            self.gui.init_all_dnd()            
            
            if self.legalmove == 0:                
                # legalmove = 0 means human made illegal move
                # must try again - still his go
                pass                                    
            elif self.legalmove == 2000:
                # double jump not yet completed - still human's go                
                self.src = self.dst     
                self.dst = 0
                self.board.display_board()
            else:
                #self.gui.set_status_bar_msg(self.get_side_to_move_msg())
                self.set_panel_msg2()
                # Move was OK - now let the other side move
                # If human is playing both sides (i.e. computer is off) then return
                # now without calling the engine
                if self.side_to_move == RED and self.red_player != COMPUTER:                    
                    return

                if self.side_to_move == WHITE and self.white_player != COMPUTER:                    
                    return                

                # It's the computers turn to move
                # kick off a separate thread for computers move so that gui is still useable                              
                GLib.timeout_add(1000, self.comp_move)  
                return

    def comp_move(self):
        self.ct= _thread.start_new_thread( self.computer_move, () )

    def computer_move(self):
        try:
            
            self.start_time = time.time()
            self.src = 0
            self.thinking = True
            # disable some functionality while the computer is thinking
            GLib.idle_add(self.gui.disable_menu_items)
            
            GLib.idle_add(self.gui.set_status_bar_msg, self.get_side_to_move_msg() + '...')
            
            GLib.timeout_add(200, self.running_display)
                       
            pre_board = self.board.get_board_position()
            self.pre_board = pre_board[:]

            # Call the engine to make computers move
            engine.setcomputercolour(self.side_to_move)            
            board_position = engine.cmove(str(self.side_to_move))
            self.board.set_board_position(board_position)

            post_board = self.board.get_board_position()
            self.post_board = post_board[:]            

            # enable functionality previously disabled
            GLib.idle_add(self.gui.enable_menu_items)
            self.thinking = False            

            # display updated board
          # for jumps use cpath
            self.cpath = self.board.get_jumps()               
            if self.cpath[0] != 0:
                time.sleep(0.5) 
                pbp = self.pre_board[:]
                self.board.set_board_position(pbp)
            GObject.timeout_add(200, self.show_computer_move, 0)                                              
        except:
            traceback.print_exc()

    def get_colour(self, colour):
        if colour == WHITE:
            return 'White'
        else:
            return 'Red'
        
        
    # print text version of board (for debugging)
    def print_board(self):
        bp = self.board.board_position
        print(" ", bp[37], " ", bp[38], " ", bp[39], " ",bp[40])
        print(bp[32], " ", bp[33], " ", bp[34], " ",bp[35])
        print(" ", bp[28], " ", bp[29], " ", bp[30], " ",bp[31])
        print(bp[23], " ", bp[24], " ", bp[25], " ",bp[26])
        print(" ", bp[19], " ", bp[20], " ", bp[21], " ",bp[22])
        print(bp[14], " ", bp[15], " ", bp[16], " ",bp[17])
        print(" ", bp[10], " ", bp[11], " ", bp[12], " ",bp[13])
        print(bp[5], " ", bp[6], " ", bp[7], " ",bp[8])
        print()

    
    # shows the computers progress as it works out the next move
    def running_display(self):        
        # get running display        
        rd = engine.rdisp()
        self.gui.set_panel_text(rd)
        if self.thinking:
            return True
        else:            
            return False 
                    
    def show_computer_move(self, jumpidx):
   
        cp = self.cpath[:]        

        # if jumped then display each move of the jump with pause between
        
        if self.cpath[0] != 0:
            time.sleep(0.5) 
   
            i = jumpidx    
     
            self.board.move_piece(cp[i], cp[i + 1])
            self.board.display_board()
            # All jumps shown
            if cp[i + 1] == 0:                    
                post_b = self.post_board[:]
                self.board.set_board_position(post_b)                   
            else:
                # more jumps - display next jump
                GObject.timeout_add(200, self.show_computer_move, i + 1)
                return
        end_time = time.time()
        elapsed = end_time - self.start_time

        # if the computers move was too fast, slow it down a bit (by 1 sec)                  
        if elapsed < 1.0:
            time.sleep(1)            
  
        self.board.display_board()
        
        self.src = 0
        self.dst = 0  

        GLib.idle_add(self.check_for_gameover)

        self.gui.init_all_dnd()

        return False    

    def check_for_gameover(self):
        if self.gameover == WHITE:
            text = "Game Over - White Wins"
            if self.white_player != COMPUTER:            
                self.gui.set_panel_text(text)
            else:
                self.gui.append_panel_text(text)
            self.gui.set_status_bar_msg(text)
        elif self.gameover == RED:
            text = "Game Over - Red Wins"
            if self.red_player != COMPUTER:
                self.gui.set_panel_text(text)
            else:
                self.gui.append_panel_text(text)
            self.gui.set_status_bar_msg(text)
        else:
            # set side to move msg in status bar                  
            self.gui.set_status_bar_msg(self.get_side_to_move_msg())             

    # get msg to show in status bar
    def get_side_to_move_msg(self):        
        
        if self.side_to_move == RED:
            msg = 'Red'
            player = self.red_player
        else:
            msg = 'White'
            player = self.white_player

        if player == HUMAN:
            msg = msg + '/Human to move'
        else:
            msg = msg + '/Computer to move'        

        return msg       


    def set_level(self, w, data):        
        self.level = data.get_current_value()                       
        engine.setlevel(data.get_current_value(), self.custom_search_depth, self.custom_time_limit)   
   

    def set_computer_player(self, w ,data):
        # 0 - Computer plays White
        # 1 - Computer plays Red
        # 2 - Computer plays White and Red
        # 3 - Computer Off
        option = data.get_current_value()
        if option == 0:
            self.white_player = COMPUTER 
            self.red_player = HUMAN                       
        elif option == 1:
            self.white_player = HUMAN
            self.red_player = COMPUTER 
        elif option == 2:
            self.white_player = COMPUTER
            self.red_player = COMPUTER  
        else:
            self.white_player = HUMAN
            self.red_player = HUMAN           

        if self.side_to_move == RED and self.red_player == COMPUTER:
            engine.setcomputercolour(RED)
        elif self.side_to_move == WHITE and self.white_player == COMPUTER:
            engine.setcomputercolour(WHITE)

        self.set_panel_msg2()
        self.gui.init_all_dnd()

    def flip_the_board(self, w):
        
        # if ToggleAction widget is checked then set flipped to True else false
        if w.get_active():
            flipped = 1
        else:
            flipped = 0

        # 0 means normal board (not flipped)
        # 1 means inverted board (flipped)        
        board_position = engine.flipboard(flipped)        
        self.board.set_board_position(board_position)                      
        self.board.display_board()        
        self.gui.init_all_dnd()        

    #
    # save users settings at program termination
    #    
    def save_settings(self):             
                
        # get settings
        s = Settings()
        s.name = NAME
        s.version = VERSION
        s.action_settings = self.gui.get_action_settings()
        s.custom_search_depth = self.custom_search_depth
        s.custom_time_limit = self.custom_time_limit
        #(s.board_size_width, s.board_size_height, s.board_size_font_size) = self.board.get_board_size()
        (s.window_width, s.window_height) = self.gui.get_main_window_size()        
        s.font_size = 8
            
        # pickle and save settings
        try:                        
            settings_file = os.path.join (self.sampath, "settings")            
            f = open(settings_file, 'wb')            
            pickle.dump(s, f)            
            f.close()        
        except AttributeError as ae:
            print("attribute error:",ae)
        except pickle.PickleError as pe:
            print("PickleError:", pe)
        except pickle.PicklingError as pe2:
            print("PicklingError:", pe2)
        except Exception as exc:
            print("cannot save settings:", exc)         

    #
    # restore users settings at program start-up
    #    
    def restore_settings(self):   
        x = ''               
        try:            
            settings_file = os.path.join (self.sampath, "settings")            
            f = open(settings_file, 'rb')            
            x = pickle.load(f)           
            f.close()        
        except EOFError as eofe:
            print("eof error:",eofe)        
        except pickle.PickleError as pe:
            print("pickle error:", pe)
        except IOError as ioe:
            pass    # Normally this error means it is the 1st run and the settings file does not exist        
        except Exception as exc:
            print("Cannot restore settings:", exc)             

        if x:
            try:
                self.custom_search_depth = x.custom_search_depth
                self.custom_time_limit = x.custom_time_limit           
                #self.board.resize_board(None, x.board_size_width, x.board_size_height, x.board_size_font_size)                           
                self.gui.set_window_size(x.window_width, x.window_height)

                #for (ag_name, setting, active) in x.action_settings:
                for tup in x.action_settings:
                    # activate the menu option                                                           
                    self.gui.activate(tup)                    
            except Exception as e:                        
                print("Not all settings were restored")                          
                  

    #
    # Callback Functions
    #
    
    def quit_game(self, b):

        self.save_settings()
        Gtk.main_quit()


    def new_game(self, b):                       
        bp = engine.newgame()
        self.board.set_board_position(bp)         
        self.board.display_board()
        self.gui.set_panel_text("Red to Move")
        # set status bar msg
        self.gui.set_status_bar_msg(self.get_side_to_move_msg())        
        self.gui.init_all_dnd()


    # Load Board Position from a previously saved game
    # The file to be loaded must be in pdn format with a .pdn extension
    def load_game(self, b):       
        
        dialog = Gtk.FileChooserDialog("Load..",
                               None,
                               Gtk.FileChooserAction.OPEN,
                               (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
                                Gtk.STOCK_OPEN, Gtk.ResponseType.OK))
        dialog.set_default_response(Gtk.ResponseType.OK)
        dialog.set_current_folder(os.path.expanduser("~"))

        filter = Gtk.FileFilter()  
        filter.set_name("pdn files")      
        filter.add_pattern("*.pdn")        
        dialog.add_filter(filter)

        filter = Gtk.FileFilter()
        filter.set_name("All files")
        filter.add_pattern("*")
        dialog.add_filter(filter)        

        response = dialog.run()
        if response == Gtk.ResponseType.OK:                                    
            self.board.board_position = engine.loadgame(dialog.get_filename())                                
            self.board.display_board()
            self.set_panel_msg()
                    
        dialog.destroy()

    # Save Board Position to a file
    # The file is saved in pdn format with a .pdn extension
    def save_game(self, b):        

        dialog = Gtk.FileChooserDialog("Save..",
                               None,
                               Gtk.FileChooserAction.SAVE,
                               (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
                                Gtk.STOCK_SAVE, Gtk.ResponseType.OK))
        dialog.set_default_response(Gtk.ResponseType.OK)
        dialog.set_current_folder(os.path.expanduser("~"))

        filter = Gtk.FileFilter()  
        filter.set_name("pdn files")      
        filter.add_pattern("*.pdn")        
        dialog.add_filter(filter)

        filter = Gtk.FileFilter()
        filter.set_name("All files")
        filter.add_pattern("*")
        dialog.add_filter(filter)        

        response = dialog.run()
        if response == Gtk.ResponseType.OK:
            filename = dialog.get_filename()
            if filename[-4:] != '.pdn':
                filename = filename + '.pdn'            
            engine.savegame(filename)        
        dialog.destroy()


    # Interrupt the computers thinking and make it move now
    def move_now(self, b):                
        engine.movenow()


        # Copy the board position to the clipboard in std FEN format     
    def copy_FEN_to_clipboard(self, b):                

        # get the clipboard
        clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)

        # get the FEN data board position from the engine        
        text = engine.getFEN()

        # put the FEN data on the clipboard
        clipboard.set_text(text, -1)

        # make our data available to other applications
        clipboard.store()

    # Paste in a FEN board position 
    def paste_FEN_from_clipboard(self, b):        

        # get the clipboard
        clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)

        # read the clipboard FEN data. 
        text = clipboard.wait_for_text()

        # Call the engine to set the board position from the FEN data
        bp = engine.setposfromFEN(text)
        self.board.set_board_position(bp)                
        self.board.display_board()
        self.set_panel_msg()
        

    # Copy the board moves to the clipboard in std PDN format
    def copy_PDN_to_clipboard(self, b): 
        
        # get the clipboard
        clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)

        # get the moves PDN moves from the engine.
        text = engine.PDNtoCB()        

        # put the PDN data on the clipboard
        clipboard.set_text(text, -1)

        # make our data available to other applications
        clipboard.store()

    # Paste in moves from PDN format
    def paste_PDN_from_clipboard(self, b):        

        # get the clipboard
        clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)

        # read the clipboard PDN data. 
        text = clipboard.wait_for_text()

        # Call the engine to set the board position from the PDN data
        bp = engine.getPDN(text)
        self.board.set_board_position(bp)        
        self.board.display_board()
        self.set_panel_msg()        
 

    def set_custom_search_depth(self, b):   
        
        dialog = Gtk.MessageDialog(
            None,  
            Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT,  
            Gtk.MessageType.QUESTION,  
            Gtk.ButtonsType.OK_CANCEL,  
            None)
        
        markup = "<b>Set User-Defined Level</b>"
        dialog.set_markup(markup)

        #create the text input fields  
        entry = Gtk.Entry() 
        entry.set_text(str(self.custom_search_depth))
        entry.set_max_length(2)
        entry.set_width_chars(5)
        
        entry2 = Gtk.Entry() 
        entry2.set_text(str(self.custom_time_limit))
        entry2.set_max_length(5)
        entry2.set_width_chars(5)

        #allow the user to press enter to do ok  
        entry.connect("activate", self.response_to_dialog, dialog, Gtk.ResponseType.OK) 
        entry2.connect("activate", self.response_to_dialog, dialog, Gtk.ResponseType.OK)  

        tbl = Gtk.Table(2, 2, True)
        tbl.attach(Gtk.Label(label="Search Depth\n(max 52)\n"), 0, 1, 0, 1)
        tbl.attach(entry, 1, 2, 0, 1)
        tbl.attach(Gtk.Label(label="Time Limit\n(in seconds)"), 0, 1, 1, 2)
        tbl.attach(entry2, 1, 2, 1, 2)
  
        #some secondary text
        markup = 'Enter values for Search Depth\n'
        markup += 'and Time Limit.'        
        dialog.format_secondary_markup(markup)
        
        dialog.vbox.add(tbl)
        dialog.show_all()  

        # If user hasn't clicked on OK then exit now
        if dialog.run() != Gtk.ResponseType.OK:
            dialog.destroy()
            return

        # user clicked OK so update with the values entered
        depth = entry.get_text()
        time = entry2.get_text()    
        dialog.destroy()
        
        # if valid number entered then set search depth
        try:
            d = int(depth)
            if d >= 0 and d <= 52:
                self.custom_search_depth = d                
        except ValueError:
            pass        
        
        # if valid number entered then set time limit
        try:
            t = int(time)
            if t >= 0:
                self.custom_time_limit = t                
        except ValueError:
            pass

        # if using user defined level then call engine with the new settings
        if self.level == 3:
            engine.setlevel(self.level, self.custom_search_depth, self.custom_time_limit)    
   
    def response_to_dialog(self, entry, dialog, response):            
        dialog.response(response)    


    # This callback quits the program
    def delete_event(self, widget, event, data=None):
        self.save_settings()        
        Gtk.main_quit()
        return False


    # process key presses
    def key_press_event(self, widget, event):        
        kp = Gdk.keyval_name(event.keyval)                   
        kp = kp.lower()
        
        # treat ctrl+= same as ctrl++ (i.e. increase board size)
        if kp == "equal" and event.get_state() & Gdk.ModifierType.CONTROL_MASK:            
            self.board.resize_board(widget)
            return        
         
        # If in position edit mode don't allow key presses except 'Delete'
        if self.board.get_pos_edit():
            if kp == 'delete':
                self.board.position_edit_clear_board()
            return

        # if computer is thinking don't allow key presses except for 'm'
        if self.thinking:
            # 'm' to interrupt computers thinking and make it move now  
            if kp == "m" or kp == "g":
                engine.movenow()
            return

        if kp == "r":
            self.board.retract()
        elif kp == "bracketleft":
            self.board.rewind()
        elif kp == "bracketright":
            self.board.forward()
        elif kp == "braceleft":
            self.board.rewind_to_start()
        elif kp == "braceright":
            self.board.forward_to_end()
        elif kp == "g":
            self.go()        
        elif kp == "2" or kp == "3" or kp == "4" or kp == "6" or kp == "k" or kp == "s":
            if kp == "2": kp = 2
            if kp == "3": kp = 3
            if kp == "4": kp = 4
            if kp == "6": kp = 6    
            if kp == "c": kp = 7
            if kp == "s": kp = 8
            # add/delete/save from opening book            
            opening_book_path = os.path.expanduser("~") + "/.samuel/opening.gbk"          
            engine.openingbook(kp, opening_book_path)  
            msg = engine.rdisp()          
            self.gui.set_panel_text(msg)
    

    # Process Button Presses
    def callback(self, widget, data=None):                       
        if data == "<":  
            # rewind board 1 move
            self.board.rewind()            
        elif data == ">":
            # forward board 1 move
            self.board.forward()            
        elif data == "<|":            
            # rewind board to start of game              
            self.board.rewind_to_start()                   
        elif data == "|>":  
            # forward board to end of game
            self.board.forward_to_end()            
        elif data == "Retract":            
            # retract the last move
            self.board.retract()            
        elif data == "Go":
            self.go()            
        elif data == "Cancel":            
            # Cancel any changes made in position edit and revert to previous position
            self.board.pos_edit_cancel()            
        elif data == "OK":            
            # Apply any changes made in position edit
            self.board.pos_edit_ok()            
    

    # 'Go' button was clicked or 'g' key was pressed
    def go(self):
        if self.gameover != 0:
            return
        if self.side_to_move == RED and self.red_player != COMPUTER:                    
            return
        if self.side_to_move == WHITE and self.white_player != COMPUTER:                    
            return                             
        # computer is thinking about next move - force it to move now            
        if self.thinking:
            engine.movenow()
            return            
            
        # engine is stopped and it's the computers turn to move. start thread to make the move        
        self.ct = _thread.start_new_thread( self.computer_move, () )             


    def set_panel_msg(self):        

        if self.gameover == RED:
            text = "Gameover - red wins"            
            self.gui.set_panel_text(text)
            self.gui.set_status_bar_msg(text)
        elif self.gameover == WHITE:
            text = "Gameover - white wins" 
            self.gui.set_panel_text(text)
            self.gui.set_status_bar_msg(text)
        elif self.side_to_move == WHITE:
            text = "White to Move\n"
            #self.gui.set_status_bar_msg("White to Move")
            self.gui.set_status_bar_msg(self.get_side_to_move_msg())            
            if self.white_player == COMPUTER:
                text = text + "Press Go to restart the game from here\n"
            else:
                text = text + "Move a white piece to restart the game from here\n"
            text = text + "Press the < > buttons to browse the game"            
            self.gui.set_panel_text(text)            
        elif self.side_to_move == RED:
            text = "Red to Move\n"
            #self.gui.set_status_bar_msg("Red to Move")
            self.gui.set_status_bar_msg(self.get_side_to_move_msg())            
            if self.red_player == COMPUTER:
                text = text + "Press Go to restart the game from here\n"
            else:
                text = text + "Move a red piece to restart the game from here\n"
            text = text + "Press the < > buttons to browse the game"            
            self.gui.set_panel_text(text)          

    def set_panel_msg2(self):
        
        # get caller       
        #print inspect.stack()[1][3]
        #print inspect.stack()[2][3]

        if self.gameover == RED:
            text = "Gameover - red wins"            
            self.gui.set_panel_text(text)
            self.gui.set_status_bar_msg(text)
        elif self.gameover == WHITE:
            text = "Gameover - white wins" 
            self.gui.set_panel_text(text)
            self.gui.set_status_bar_msg(text)
        elif self.side_to_move == WHITE:
            #text = "White to Move\n"
            #self.gui.set_status_bar_msg("White to Move")
            self.gui.set_status_bar_msg(self.get_side_to_move_msg())            
            #if self.white_player == COMPUTER:
            #    text = text + "Press Go to restart the game from here\n"
            #else:
            #    text = text + "Move a white piece to restart the game from here\n"
            #text = text + "Press the < > buttons to browse the game"
            text = "White to Move"            
            self.gui.set_panel_text(text)            
        elif self.side_to_move == RED:
            #text = "Red to Move\n"
            #self.gui.set_status_bar_msg("Red to Move")
            self.gui.set_status_bar_msg(self.get_side_to_move_msg())            
            #if self.red_player == COMPUTER:
            #    text = text + "Press Go to restart the game from here\n"
            #else:
            #    text = text + "Move a red piece to restart the game from here\n"
            #text = text + "Press the < > buttons to browse the game"
            text = "Red to Move"            
            self.gui.set_panel_text(text)  

    def update_board_status(self, side_to_move, legalmove, gameover):
        self.side_to_move = side_to_move
        self.legalmove = legalmove             
        self.gameover = gameover 

    def get_src(self):
        return self.src

    def get_prefix(self):
        return self.prefix

    def set_side_to_move(self, side_to_move):
        self.side_to_move = side_to_move

    def get_side_to_move(self):
        return self.side_to_move

    def get_players(self):
        return self.red_player, self.white_player

    def open_help(self, w):
        try:        
            webbrowser.open("http://www.johncheetham.com/projects/samuel/help/help.html")
        except Exception as e:
            pass

# class to save settings on program exit and restore on program start
class Settings:
    pass

def run():
    Game()
    GObject.threads_init()
    Gtk.main()
    return 0     

if __name__ == "__main__":        
    run()


