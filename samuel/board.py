#
#   board.py
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

from gi.repository import Gtk, Gdk, GdkPixbuf
import os.path
import sys, _thread, time
if os.path.abspath(os.path.dirname(__file__)).startswith(sys.prefix):
    from samuel import engine
else:
    import engine
from .constants import *


class Board:

    def __init__(self, board_position):
        self.board_position = board_position 

  
    def build_board(self):        

        #
        # The guicheckers engine has numbers for the squares like this:
        #
        #            37  38  39  40
        #          32  33  34  35
        #            28  29  30  31
        #          23  24  25  26
        #            19  20  21  22
        #          14  15  16  17
        #            10  11  12  13
        #           5   6   7   8  
        #
        # So for example the square at (x,y) = (3,0) is referred to as 38 in the engine.
        # The board_squares are used to convert from one form to the other.
        #

        self.board_squares = [
                    (37, 1, 0), (38, 3, 0), (39, 5, 0), (40, 7, 0),         
                    (32, 0, 1), (33, 2, 1), (34, 4, 1), (35, 6, 1),
                    (28, 1, 2), (29, 3, 2), (30, 5, 2), (31, 7, 2),        
                    (23, 0, 3), (24, 2, 3), (25, 4, 3), (26, 6, 3),
                    (19, 1, 4), (20, 3, 4), (21, 5, 4), (22, 7, 4),        
                    (14, 0, 5), (15, 2, 5), (16, 4, 5), (17, 6, 5),
                    (10, 1, 6), (11, 3, 6), (12, 5, 6), (13, 7, 6),        
                    (5, 0, 7), (6, 2, 7), (7, 4, 7), (8, 6, 7)]  

        #
        # Each board_square has a corresponding content
        #
        #   0 is unoccupied
        #   1 is a red piece   
        #   2 is a white piece
        #   5 is a red king
        #   6 is a white king 
        #
        # This sets the initial board position
        # Initial board position is returned by the engine 
        #self.board_position = [8, 8, 8, 8, 8, 1, 1, 1, 1, 8, 1, 1, 1, 1, 1, 1, 1, 1, 8, 0, 0, 0, \
        #                 0, 0, 0, 0, 0, 8, 2, 2, 2, 2, 2, 2, 2, 2, 8, 2, 2, 2, 2, 8, 8, 8, 8, 8, 8, 8, 0, 0, 1]          

        self.pos_edit = False
        
        # Numbers used to represent pieces on the board (as returned by the guicheckers engine)
        self.not_occupied = 0
        self.red_piece = 1
        self.red_king = 5 
        self.white_piece = 2
        self.white_king = 6 

        self.init_board() 
        self.display_board()             


    def set_refs(self, game, gui):
        self.game = game
        self.gui = gui        

    
    def set_board_status(self):

        # The last 3 values passed back are side_to_move, legalmove and gameover.        

        # side to move next. 2 = white, 1 = red        
        side_to_move = self.board_position[51]
 
        # legalmove. 1 = legal, 0 = not legal        
        legalmove = self.board_position[50]

        # gameover. 0 = false, 1 = gameover (white wins), 2 = game over (red wins)        
        gameover = self.board_position[49]

        # for jumps cpath contains up to 8 positions
        cpath = self.board_position[52:62]

        # set variables in game
        self.game.update_board_status(side_to_move, legalmove, gameover)


    def display_board(self):        

        #for sq in self.board_squares:
        #    gc_loc, x, y = sq
        #    self.setpiece(gc_loc, x, y)

        self.set_board_status()
        self.gui.draw_board()
           

    def get_piece_at_square(self, x, y):
        for sq in self.board_squares:
            gc_loc, sx, sy = sq
            if sx == x and sy == y:
                return self.board_position[gc_loc]
        print("error in board.py get_piece_at_square") 
        sys.exit()      


    #
    # get_gc_loc
    #
    # Converts a square x,y location to the integer representation
    # expected by the guicheckers engine    
    #       
    def get_gc_loc(self, x, y):
        
        for sq in self.board_squares:
            #print sq
            gc_loc, sqx, sqy = sq
            if (sqx, sqy) == (x, y):
                return gc_loc

        print("get_gc_loc error")
        print("x,y=",x,y)
        sys.exit()

    #
    # Converts a gc_loc into the x,y square position   
    #
    def get_x_y(self, gc_loc):
        for sq in self.board_squares:
            #print sq
            loc, sqx, sqy = sq
            if loc == gc_loc:
                return sqx, sqy
        print("get_x_y error")
        print("gc_loc=",gc_loc)
        sys.exit()


    def init_board(self):       
        
        self.font_size = 8      

        prefix = self.game.get_prefix()

        
        # loop through all squares on the board setting the correct
        # image for that square
        for x in range(0, 8):
            for y in range(0, 8): 
                found = False
                for sq in self.board_squares:
                    gc_loc, sqx, sqy = sq
                    if (sqx, sqy) == (x, y):
                        found = True                                                
                        # call gui to process this square when clicked on
                        self.gui.init_black_board_square(x, y)                        
                        break 

                if not found:
                    # if not in board squares then must be a white square (not used)
                    # call gui to show this square
                    self.gui.init_white_board_square(x, y)

    def get_board_size(self):
        return (64, 64, 8)
        #return (self.wcheck_pixbuf.get_width(), self.wcheck_pixbuf.get_height(), self.font_size)

    # Edit Board Position     
    def position_edit_init(self, b):
        
        self.pos_edit = True
        self.saved_board_position = self.board_position[:]
        text = "Use left mouse to set red pieces\n"
        text += "Use right mouse to set white pieces\n"
        text += "Set Side to Move with the radio buttons\n"
        text += "Click OK to apply changes.\n"
        text += "Click Cancel to discard changes"                      
        
        self.gui.disable_posedit_menu_items()
        self.gui.init_posedit_panel(text)        

    def position_edit(self, event, data):
        
        # get x,y co-ords of square clicked on
        x, y = data
        
        # convert the x, y co-ords into the gui checkers representation
        gc_loc = self.get_gc_loc(x, y)

        # if left click then set red pieces 
        if event.button == 1:
            if self.board_position[gc_loc] == self.red_king:
                self.board_position[gc_loc] = self.not_occupied
            elif self.board_position[gc_loc] == self.red_piece:
                self.board_position[gc_loc] = self.red_king            
            else:  
                self.board_position[gc_loc] = self.red_piece           

        # if right click then set white pieces 
        if event.button == 3:
            if self.board_position[gc_loc] == self.white_king:
                self.board_position[gc_loc] = self.not_occupied
            elif self.board_position[gc_loc] == self.white_piece:
                self.board_position[gc_loc] = self.white_king            
            else:  
                self.board_position[gc_loc] = self.white_piece
           
        self.gui.draw_board()       


    def position_edit_clear_board(self):
        #sqs= (37, 38, 39, 40,
        #    32, 33, 34, 35,
        #      28, 29, 30, 31,
        #    23, 24, 25, 26,
        #      19, 20, 21, 22,
        #    14, 15, 16, 17,
        #      10, 11, 12, 13,
        #     5,  6,  7,  8)
        for (gc_loc, x, y) in self.board_squares:            
            self.board_position[gc_loc] = self.not_occupied
            #self.setpiece(gc_loc, x, y)
            

    #
    # functions to review the game
    #

    # rewind board 1 move
    # This method is invoked by clicking the rewind button or by
    # pressing the left bracket '[' key
    def rewind(self):
        self.board_position = engine.prev()    
        self.display_board()
        self.game.set_panel_msg()
        self.gui.init_all_dnd()

    # forward board 1 move
    # This method is invoked by clicking the forward button or by
    # pressing the right bracket ']' key
    def forward(self):
        self.board_position = engine.next()
        self.display_board()
        self.game.set_panel_msg()
        self.gui.init_all_dnd()

    # rewind board to start of game
    # This method is invoked by clicking the rewind to start button or by
    # pressing the left brace '{' key
    def rewind_to_start(self):
        self.board_position = engine.start()    
        self.display_board()
        self.game.set_panel_msg()
        self.gui.init_all_dnd()

    # forward board to end of game
    # This method is invoked by clicking the forward to end button or by
    # pressing the right brace '}' key
    def forward_to_end(self):
        self.board_position = engine.end()
        self.display_board()
        self.game.set_panel_msg()
        self.gui.init_all_dnd()

    # retract the last move
    # This method is invoked by clicking the Retract button or by
    # pressing the 'r' key
    def retract(self):
        self.board_position = engine.retract()
        self.display_board()
        self.game.set_panel_msg()
        self.gui.init_all_dnd()        


    #
    # functions for position edit
    #
    
    # Cancel any changes made in position edit and revert to previous position
    def pos_edit_cancel(self):
        if self.pos_edit:                
            self.pos_edit = False
            # We are finished editing so hide the edit panel
            self.gui.hide_posedit_panel()
            self.gui.enable_posedit_menu_items()           
                
            self.board_position = self.saved_board_position
            self.display_board()
            self.game.set_panel_msg()
            

    # Apply any changes made in position edit
    def pos_edit_ok(self):
        if self.pos_edit:                
            self.pos_edit = False
            # We are finished editing so hide the edit panel
            self.gui.hide_posedit_panel()
            self.gui.enable_posedit_menu_items()          

            # pass in updated board to the engine
            # only update the squares that contain pieces (set others to -1)
            lst = []
            for sq in self.board_squares:
                gc_loc, x, y = sq 
                lst.append(gc_loc)

            temp_board = self.board_position[:]
            for i in range(0, len(temp_board)):
                if i not in lst:
                    temp_board[i] = -1                
            
            # get side to move as set in the position edit radio button
            side_to_move = self.gui.get_side_to_move()

            # set side to move in the game
            self.game.set_side_to_move(side_to_move)

            # update the engine with the side to move and new board layout
            engine.setsidetomove(side_to_move)
            self.board_position = engine.setboard(temp_board)    
            self.display_board()
            self.game.set_panel_msg()

    def get_pos_edit(self):
        if self.pos_edit:
            return True
        else:
            return False

    def valid_source_square(self, gc_loc, human_colour):        
        if human_colour == RED:
            valid_pieces = (self.red_piece, self.red_king)
        else:            
            valid_pieces = (self.white_piece, self.white_king)
        
        if self.board_position[gc_loc] in valid_pieces:            
            return True
        else:
            return False


    def valid_destination_square(self, gc_loc):
        if self.board_position[gc_loc] == self.not_occupied: 
            return True
        else:
            return False

    def set_board_position(self, bp):
        self.board_position = bp

    def get_board_position(self):
        return self.board_position

    def get_jumps(self):
        return self.board_position[52:62]    
    
    def move_piece(self, src, dst):
        piece = self.board_position[src] 
        self.board_position[src] = self.not_occupied
        self.board_position[dst] = piece

    def set_piece_at_square(self, gc_loc, piece):
        gc_loc = int(gc_loc)
        self.board_position[gc_loc] = piece

    def get_board_squares(self):
        return self.board_squares


