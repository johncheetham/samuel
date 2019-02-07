/*
    enginemodule.cpp   
   
    This file is part of Samuel

    This file contains code from the guicheckers project.
    See http://www.3dkingdoms.com/checkers.htm    

    Samuel is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Samuel is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Samuel.  If not, see <http://www.gnu.org/licenses/>.
   
 */

#include <Python.h>
#include <iostream>
#include <cstdio>

using namespace std;

// Functions in ai.cpp to be called by enginemodule
extern void InitEngine(char *openingbookpath, char *endgame2pcpath, char *endgame3pcpath, char *endgame4pcpath);
extern void HMove(int s, int d);
extern void DoComputerMove(char ch);
extern void AINewGame();  
extern void LoadGame( char *sFilename);  
extern void SaveGame( char *sFilename);
extern void CopyToFEN(char* g_buffer); 
extern void PasteFromFEN(char* g_buffer); 
extern void CopyPDNtoClipBoard(char* g_buffer); 
extern void PasteFromPDN(char* g_buffer);
extern void MovePrev(); 
extern void MoveNext(); 
extern void MoveStart();
extern void MoveEnd();
extern void Retract();
extern void actionOB(int action, char *openingbookpath);
extern void SetBoard(int idx, int piece);
extern void SetSideToMove(int stm);
extern int CheckForGameOver();
extern int flip(int src);

extern char* GetBoardSquares();
PyObject *BoardPosition();

int gameover = 0;
int legalmove = 0;
char msg[4096];
int computerLevel = 0;

#define FALSE 0
#define TRUE 1
#define WHITE 2
#define BLACK 1 // Black == Red

int BEGINNER_DEPTH = 2;
int NORMAL_DEPTH = 8;
int EXPERT_DEPTH = 52; // Max depth for each level

// GLOBAL VARIABLES... ugg, too many?
const char *g_sNameVer = "Samuel 0.1.8";
char *g_sInfo = NULL;
float fMaxSeconds = 2.0f, g_fPanic; // The number of seconds the computer is allowed to search
int g_bEndHard = TRUE; // Set to true to stop search after fMaxSeconds no matter what.
int g_MaxDepth = BEGINNER_DEPTH; 
long nodes, nodes2;
int SearchDepth, g_SelectiveDepth;
char g_cCompColor = WHITE;
clock_t starttime, endtime, lastTime = 0;
int g_SearchingMove = 0, g_SearchEval = 0;   
int g_nDouble = 0, g_nMoves = 0; // Number of moves played so far in the current game
int g_bSetupBoard, g_bThinking = false;
int bCheckerBoard = 0;
int *g_pbPlayNow, g_bStopThinking = 0;
int g_bDatabases = 0;
unsigned int HASHTABLESIZE = 700000; // 700,000 entries * 12 bytes per entry
int nSquare;
int hashing = 1;
char g_buffer2[32768]; // For PDN copying/pasting/loading/saving
char g_sideToMove = BLACK;
char cpath[10] = {0, 0, 0, 0, 0, 0, 0, 0};
int flipped = FALSE;

void DisplayText (const char *sText);

// Initialise the engine
static PyObject *
engine_init(PyObject *self, PyObject *args)
{
    char *openingbookpath;
    char *endgame2pcpath;
    char *endgame3pcpath;
    char *endgame4pcpath;
    
    // store the path to samuel config files
    if (!PyArg_ParseTuple(args, "ssss", &openingbookpath, &endgame2pcpath, &endgame3pcpath, &endgame4pcpath))
        return NULL;

    InitEngine(openingbookpath, endgame2pcpath, endgame3pcpath, endgame4pcpath);    
    return(BoardPosition());    
}

// Do Human Move
static PyObject *
engine_hmove(PyObject *self, PyObject *args)
{    
    int src,dst;
    
    if (!PyArg_ParseTuple(args, "ii", &src, &dst))
        return NULL;    
    
    if (flipped)
    {
        src = flip(src);
        dst = flip(dst);
    }

    HMove(src, dst);    
    return(BoardPosition());    
}

// Do Computer Move
static PyObject *
engine_cmove(PyObject *self, PyObject *args)
{
    const char *command;    
    
    if (!PyArg_ParseTuple(args, "s", &command))
        return NULL;
    
    for (int i = 0; i < 10; i++) cpath[i] = 0;    

    char ch=command[0];
    Py_BEGIN_ALLOW_THREADS    
    DoComputerMove(ch);    
    Py_END_ALLOW_THREADS     
    return(BoardPosition());      
}

// Set computers Skill Level
// sets search depth and time limit
static PyObject *
engine_setlevel(PyObject *self, PyObject *args)
{    
    int level;
    int uddepth;    // user defined search depth
    int timelimit;  // user defined time limit    

    if (!PyArg_ParseTuple(args, "iii", &level, &uddepth, &timelimit))
        return NULL;        

    g_bEndHard = FALSE;
    
    if (level == 0)
    {
        g_MaxDepth = BEGINNER_DEPTH;
        fMaxSeconds = 2.0f;        
    }
    else if (level == 1)
    {
        g_MaxDepth = NORMAL_DEPTH;
        fMaxSeconds = 7.0f;        
    }
    else if (level == 2)
    {
        g_MaxDepth = EXPERT_DEPTH;
        fMaxSeconds = 30.0f;        
    }
    else if (level == 3)
    {
        g_MaxDepth = uddepth;
        fMaxSeconds = timelimit;
        g_bEndHard = TRUE;        
    }
    computerLevel = level;    
    Py_RETURN_NONE;
}

// Restart Game
static PyObject *
engine_newgame(PyObject *self, PyObject *args)
{
    AINewGame();
    return(BoardPosition());     
}

// Load a saved game from file
static PyObject *
engine_loadgame(PyObject *self, PyObject *args)
{    
    char *sFilename;     
    if (!PyArg_ParseTuple(args, "s", &sFilename))
        return NULL;
    
    LoadGame(sFilename);    
    return(BoardPosition());
}

// Save the game to a file
static PyObject *
engine_savegame(PyObject *self, PyObject *args)
{    
    char *sFilename;     
    if (!PyArg_ParseTuple(args, "s", &sFilename))
        return NULL;    

    SaveGame(sFilename);
    Py_RETURN_NONE;
}

// Get the game position (FEN format)
// Pass it back to python calling program so it can be copied to the clipboard
static PyObject *
engine_getFEN(PyObject *self, PyObject *args)
{       
    CopyToFEN(g_buffer2);     
    return Py_BuildValue("s", g_buffer2);
}

// Set the game position from a FEN format passed in by python calling program
static PyObject *
engine_set_pos_from_FEN(PyObject *self, PyObject *args)
{
    char *sFEN;
         
    if (!PyArg_ParseTuple(args, "s", &sFEN))
        return NULL;
           
    PasteFromFEN(sFEN);    
    return(BoardPosition());
}

// Get the game position (PDN format)
// Pass it back to python calling program so it can be copied to the clipboard
static PyObject *
engine_copyPDNtoCB(PyObject *self, PyObject *args)
{    
    CopyPDNtoClipBoard(g_buffer2);     
    return Py_BuildValue("s", g_buffer2);
}

static PyObject *
engine_getPDN(PyObject *self, PyObject *args)
{    
    char *sPDN;
         
    if (!PyArg_ParseTuple(args, "s", &sPDN))
        return NULL;

    PasteFromPDN(sPDN);    
    return(BoardPosition());            
}

static PyObject *
engine_movenow(PyObject *self, PyObject *args)
{    
    g_bStopThinking = true;    
    Py_RETURN_NONE;
}

static PyObject *
engine_running_display(PyObject *self, PyObject *args)
{    
    return Py_BuildValue("s", msg);
}

static PyObject *
engine_prev(PyObject *self, PyObject *args)
{     
    MovePrev();    
    return(BoardPosition());            
}

static PyObject *
engine_next(PyObject *self, PyObject *args)
{     
    MoveNext();    
    return(BoardPosition());            
}

static PyObject *
engine_start(PyObject *self, PyObject *args)
{      
    MoveStart();    
    return(BoardPosition());                
}

static PyObject *
engine_end(PyObject *self, PyObject *args)
{    
    MoveEnd();    
    return(BoardPosition());         
}

static PyObject *
engine_retract(PyObject *self, PyObject *args)
{        
    Retract();    
    return(BoardPosition());         
}

static PyObject *
engine_opening_book(PyObject *self, PyObject *args)
{    
    int OBaction;
    char *openingbookpath;
         
    if (!PyArg_ParseTuple(args, "is", &OBaction, &openingbookpath))
        return NULL;    
    
    actionOB(OBaction, openingbookpath);
    Py_RETURN_NONE;    
}

static PyObject *
engine_setboard(PyObject *self, PyObject *args)
{
    PyObject * boardlistobj; /* the list of board positions */
    PyObject *item;    
    int listSize;            /* number of items in the list */

    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &boardlistobj))
        return NULL;
       
    /* get the number of lines passed to us */
    listSize = PyList_Size(boardlistobj);    

    /* indexes to be updated
        #            37  38  39  40
        #          32  33  34  35
        #            28  29  30  31
        #          23  24  25  26
        #            19  20  21  22
        #          14  15  16  17
        #            10  11  12  13
        #           5   6   7   8  
    */

    /* iterate over items of the list for numbers */
    for (int i=0; i<listSize; i++)
    {    	
    	
        item = PyList_GetItem(boardlistobj, i);
        if (!PyLong_Check(item))
            return NULL; /* return error if non integer */

        int piece = PyLong_AsLong(item);        

        // only update squares which contain a piece
        if (piece != -1)        
        {
            int sq = i;
            if (flipped) sq = flip(i);        
            SetBoard(sq, piece);
        }        
    }
    return(BoardPosition());    
}

static PyObject *
engine_set_side_to_move(PyObject *self, PyObject *args)
{    
    int stm;    
         
    if (!PyArg_ParseTuple(args, "i", &stm))
        return NULL;    
    
    SetSideToMove(stm);
    Py_RETURN_NONE;    
}

static PyObject *
engine_set_computer_colour(PyObject *self, PyObject *args)
{    
    int comp_colour;    
         
    if (!PyArg_ParseTuple(args, "i", &comp_colour))
        return NULL;    
    
    g_cCompColor = comp_colour;
    Py_RETURN_NONE;    
}

static PyObject *
engine_flip_board(PyObject *self, PyObject *args)
{    
    int flip;    
         
    if (!PyArg_ParseTuple(args, "i", &flip))
        return NULL;    
    
    flipped = flip;
    return(BoardPosition());       
}

// Get Board Position as a Python List
PyObject *BoardPosition()
{
    char *pos = 0; 
    pos = GetBoardSquares();
    int pos_len=62;
    gameover = CheckForGameOver();
    PyObject *lst = PyList_New(pos_len);    

    // If board has been flipped then return the pieces upside-down
    if (flipped)
    {
        lst = Py_BuildValue("[iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii]",   \
            pos[0],pos[1],pos[2],pos[3],pos[4],pos[40],pos[39],pos[38],pos[37],pos[9],pos[35],pos[34],pos[33],pos[32], \
            pos[31],pos[30],pos[29],pos[28],pos[18],pos[26],pos[25],pos[24],pos[23],pos[22],       \
            pos[21],pos[20],pos[19],pos[27],pos[17],pos[16],pos[15],pos[14],pos[13],pos[12],       \
            pos[11],pos[10],pos[36],pos[8],pos[7],pos[6],pos[5],pos[41],pos[42],pos[43],       \
            pos[44],pos[45],pos[46],pos[47],pos[48], \
            gameover, legalmove, g_sideToMove, flip(cpath[0]), flip(cpath[1]), flip(cpath[2]), flip(cpath[3]),  \
            flip(cpath[4]), flip(cpath[5]), flip(cpath[6]), flip(cpath[7]), flip(cpath[8]), flip(cpath[9]) \
        );
    }
    else
    {
        lst = Py_BuildValue("[iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii]",   \
            pos[0],pos[1],pos[2],pos[3],pos[4],pos[5],pos[6],pos[7],pos[8],pos[9],pos[10],pos[11],pos[12],pos[13], \
            pos[14],pos[15],pos[16],pos[17],pos[18],pos[19],pos[20],pos[21],pos[22],pos[23],       \
            pos[24],pos[25],pos[26],pos[27],pos[28],pos[29],pos[30],pos[31],pos[32],pos[33],       \
            pos[34],pos[35],pos[36],pos[37],pos[38],pos[39],pos[40],pos[41],pos[42],pos[43],       \
            pos[44],pos[45],pos[46],pos[47],pos[48], \
            gameover, legalmove, g_sideToMove, cpath[0], cpath[1], cpath[2], cpath[3], cpath[4], \
            cpath[5], cpath[6], cpath[7], cpath[8], cpath[9] \
        );
    }
    return lst;   
} 

void DisplayText(const char *sText)
{
    cout << sText << endl;
}

static PyMethodDef EngineMethods[] = {    
    {"init", engine_init, METH_VARARGS, "Initialise the engine."},
    {"hmove", engine_hmove, METH_VARARGS, "Human Move."},
    {"cmove", engine_cmove, METH_VARARGS, "Computer Move."}, 
    {"setlevel", engine_setlevel, METH_VARARGS, "Set Level."},          
    {"newgame", engine_newgame, METH_VARARGS, "New Game."}, 
    {"loadgame", engine_loadgame, METH_VARARGS, "Load Game."},     
    {"savegame", engine_savegame, METH_VARARGS, "Save Game."},
    {"getFEN", engine_getFEN, METH_VARARGS, "Get FEN."},  
    {"setposfromFEN", engine_set_pos_from_FEN, METH_VARARGS, "Set board position from FEN."}, 
    {"PDNtoCB", engine_copyPDNtoCB, METH_VARARGS, "Copy PDN to clipboard."}, 
    {"getPDN", engine_getPDN, METH_VARARGS, "Get PDN."}, 
    {"movenow", engine_movenow, METH_VARARGS, "Move Now."}, 
    {"rdisp", engine_running_display, METH_VARARGS, "Running Display."}, 
    {"prev", engine_prev, METH_VARARGS, "Previous Move."}, 
    {"next", engine_next, METH_VARARGS, "Next Move."},  
    {"start", engine_start, METH_VARARGS, "Start of Moves."},
    {"end", engine_end, METH_VARARGS, "End of Moves."},  
    {"retract", engine_retract, METH_VARARGS, "Retract Move."}, 
    {"openingbook", engine_opening_book, METH_VARARGS, "Opening Book."},
    {"setboard", engine_setboard, METH_VARARGS, "Set Board."},   
    {"setsidetomove", engine_set_side_to_move, METH_VARARGS, "Set Side To Move."}, 
    {"setcomputercolour", engine_set_computer_colour, METH_VARARGS, "Set Computer Colour."},
    {"flipboard", engine_flip_board, METH_VARARGS, "Flip Board."},                           
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef enginedef = {
    PyModuleDef_HEAD_INIT,
    "enginemodule",      /* m_name */
    "This is the engine module",  /* m_doc */
    -1,                  /* m_size */
    EngineMethods,       /* m_methods */
    NULL,                /* m_reload */
    NULL,                /* m_traverse */
    NULL,                /* m_clear */
    NULL,                /* m_free */
};

PyMODINIT_FUNC PyInit_engine(void)
{
    return PyModule_Create(&enginedef);
}

