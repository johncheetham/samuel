/*
    movegen.cpp   
   
    This file is part of Samuel

    This file comes from the guicheckers project.
    See http://www.3dkingdoms.com/checkers.htm

    It has been amended for use in Samuel.

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

#define UINT unsigned int

#define WHITE 2
#define BLACK 1 // Black == Red
#define EMPTY 0
#define BPIECE 1
#define WPIECE 2
#define KING 4
#define BKING  5
#define WKING  6
#define INVALID 8
#define NONE 255
#define TIMEOUT 31000
#define DOUBLEJUMP 2000
#define HUMAN 128
#define MAKEMOVE 129
#define SEARCHED 127
#define MAX_SEARCHDEPTH 96
#define MAX_GAMEMOVES 2048
#define KING_VAL 33

// Convert between 32 square board and internal 44 square board
int BoardLocTo32[48] = { 0, 0, 0, 0, 0, 0, 1, 2, 3, 0, 4, 5, 6, 7, 8,  9, 10, 11, 0, 12, 13, 14, 15, 16, 17, 18, 19, 0, 20, 21, 22, 23, 24, 25, 26, 27, 0, 28, 29, 30, 31};
int BoardLoc32[32] = { 5, 6, 7, 8, 10, 11, 12, 13, 14, 15, 16, 17, 19, 20, 21, 22, 23, 24, 25, 26, 28, 29, 30, 31, 32, 33, 34, 35, 37, 38, 39, 40};

// =================================================
//
//              MOVE GENERATION 
//
// =================================================
struct SMove 
    {
     unsigned long SrcDst;
     char cPath[12];
    };

struct SCheckerBoard 
{
    unsigned long WP, BP, K;

    int GetBlackMoves( );
    int GetWhiteMoves( );
    UINT GetJumpersWhite( );
    UINT GetJumpersBlack( );
    UINT GetMoversWhite( );
    UINT GetMoversBlack( );
    void ConvertFromSqs( char sqs[] );
};

//
// BITBOARD INITILIZATION
//

//  28 29 30 31
// 24 25 26 27
//  20 21 22 23
// 16 17 18 19
//  12 13 14 15
// 08 09 10 11
//  04 05 06 07
// 00 01 02 03

UINT MASK_L3, MASK_L5, MASK_R3, MASK_R5, MASK_TOP, MASK_BOTTOM, MASK_TRAPPED_W, MASK_TRAPPED_B, MASK_2ND, MASK_7TH;
UINT MASK_EDGES, MASK_2CORNER1, MASK_2CORNER2;
UINT S[32];
unsigned char aBitCount[65536];
unsigned char aLowBit[65536];
unsigned char aHighBit[65536];

void InitBitTables()
{
    S[0] = 1;
    for (int i = 1; i < 32; i++) S[i] = S[i-1] * 2;

    MASK_L3 = S[1] | S[2] | S[3] | S[9] | S[10] | S[11] | S[17] | S[18] | S[19] | S[25] | S[26] | S[27];
    MASK_L5 = S[4] | S[5] | S[6] | S[12] | S[13] | S[14] | S[20] | S[21] | S[22];
    MASK_R3 = S[28] | S[29] | S[30] | S[20] | S[21] | S[22] | S[12] | S[13] | S[14] | S[4] | S[5] | S[6];
    MASK_R5 = S[25] | S[26] | S[27] | S[17] | S[18] | S[19] | S[9] | S[10] | S[11];
    MASK_TOP = S[28] | S[29] | S[30] | S[31] | S[24] | S[25] | S[26] | S[27] | S[20] | S[21] | S[22] | S[23];
    MASK_BOTTOM = S[0] | S[1] | S[2] | S[3] | S[4] | S[5] | S[6] | S[7] | S[8] | S[9] | S[10] | S[11];
    MASK_TRAPPED_W = S[0] | S[1] | S[2] | S[3] | S[4];
    MASK_TRAPPED_B = S[28] | S[29] | S[30] | S[31] | S[27];
    MASK_2ND = S[4] | S[5] | S[6] | S[7];
    MASK_7TH = S[24] | S[25] | S[26] | S[27];
    MASK_EDGES = S[24] | S[20] | S[16] | S[12] | S[8] | S[4] | S[0] | S[7] | S[11] | S[15] | S[19] | S[23] | S[27] | S[31];
    MASK_2CORNER1 = S[3] | S[7];
    MASK_2CORNER2 = S[24] | S[28];

    for (int Moves = 0; Moves < 65536; Moves++) {
        int nMoves = 0, nLow = 255, nHigh = 255;
        for (int i = 0; i < 16; i++) {
            if ( Moves & S[i] ) {
                nMoves++;
                if (nLow == 255) nLow = i;
                nHigh = i;
                }
        }
        aLowBit[ Moves ] = nLow;
        aHighBit[ Moves ] = nHigh;
        aBitCount[ Moves ] = nMoves;
    }
}

UINT inline BitCount( UINT Moves )
{
    if (Moves == 0) return 0;
    return aBitCount[ (Moves & 65535) ] + aBitCount[ ((Moves>>16) & 65535) ];
}

UINT inline FindLowBit( UINT Moves )
{
    if ( (Moves & 65535) ) return aLowBit[ (Moves & 65535) ];
    if ( ((Moves>>16) & 65535) ) return aLowBit[ ((Moves>>16) & 65535) ] + 16;
    return 0;
}

UINT inline FindHighBit( UINT Moves )
{
    if ( ((Moves>>16) & 65535) ) return aHighBit[ ((Moves>>16) & 65535) ] + 16;
    if ( (Moves & 65535) ) return aHighBit[ (Moves & 65535) ];
    return 0;
}

//
// BIT BOARD FUNCTIONS
//
UINT SCheckerBoard::GetMoversWhite( ) 
{
    const UINT nOcc = ~(WP | BP); // Not Occupied
    UINT Moves = (nOcc << 4);
    const UINT WK = WP&K;         // Kings
    Moves |= ((nOcc&MASK_L3) << 3);
    Moves |= ((nOcc&MASK_L5) << 5);
    Moves &= WP;
    if ( WK ) {
        Moves |= (nOcc >> 4) & WK;
        Moves |= ((nOcc&MASK_R3) >> 3) & WK;
        Moves |= ((nOcc&MASK_R5) >> 5) & WK;
        }    
    return Moves;
}

UINT SCheckerBoard::GetMoversBlack( ) 
{
    const UINT nOcc = ~(WP | BP);
    UINT Moves = (nOcc >> 4) & BP;
    const UINT BK = BP&K;
    Moves |= ((nOcc&MASK_R3) >> 3) & BP;
    Moves |= ((nOcc&MASK_R5) >> 5) & BP;
    if ( BK ) {
        Moves |= (nOcc << 4) & BK;
        Moves |= ((nOcc&MASK_L3) << 3) & BK;
        Moves |= ((nOcc&MASK_L5) << 5) & BK;
        }
    return Moves;
}

int SCheckerBoard::GetWhiteMoves( ) 
{
    const UINT nOcc = ~(WP | BP);   
    int nMoves = BitCount( (nOcc << 4) & WP );
    nMoves += BitCount( ( ((nOcc&MASK_L3) << 3) | ((nOcc&MASK_L5) << 5) ) & WP );
    const UINT WK = WP&K;   
    if ( WK ) {
        nMoves += BitCount( (nOcc >> 4) & WK );
        nMoves += BitCount( ( ((nOcc&MASK_R3) >> 3) | ((nOcc&MASK_R5) >> 5) ) & WK );
        }
    return nMoves;
}

int SCheckerBoard::GetBlackMoves( ) 
{
    const UINT nOcc = ~(WP | BP);
    int nMoves = BitCount( (nOcc >> 4) & BP );
    nMoves += BitCount( ( ((nOcc&MASK_R3) >> 3) | ((nOcc&MASK_R5) >> 5) ) & BP );
    const UINT BK = BP&K;
    if ( BK ) {
        nMoves += BitCount( (nOcc << 4) & BK );
        nMoves += BitCount( ( ((nOcc&MASK_L3) << 3) | ((nOcc&MASK_L5) << 5) ) & BK );
        }
    return nMoves;
}

UINT SCheckerBoard::GetJumpersWhite( )
{   
    const UINT nOcc = ~(WP | BP);
    const UINT WK = WP&K;
    UINT Movers = 0;
    UINT Temp = (nOcc << 4) & BP;
    if (Temp) Movers |= (((Temp&MASK_L3) << 3) | ((Temp&MASK_L5) << 5)) & WP;
    Temp = ( ((nOcc&MASK_L3) << 3) | ((nOcc&MASK_L5) << 5) ) & BP;
    Movers |= (Temp << 4) & WP;
    if (WK) {
        Temp = (nOcc>> 4) & BP;
        if (Temp) Movers |= (((Temp&MASK_R3) >> 3) | ((Temp&MASK_R5) >> 5)) & WK;
        Temp = ( ((nOcc&MASK_R3) >> 3) | ((nOcc&MASK_R5) >> 5) ) & BP;
        if (Temp) Movers |= (Temp >> 4) & WK;
        }   
    return Movers;
}

UINT SCheckerBoard::GetJumpersBlack( )
{   
    const UINT nOcc = ~(WP | BP);
    const UINT BK = BP&K;
    UINT Movers = 0;
    UINT Temp = (nOcc >> 4) & WP;
    if (Temp) Movers |= (((Temp&MASK_R3) >> 3) | ((Temp&MASK_R5) >> 5)) & BP;
    Temp = ( ((nOcc&MASK_R3) >> 3) | ((nOcc&MASK_R5) >> 5) ) & WP;
    Movers |= (Temp >> 4) & BP;
    if (BK) {
        Temp = (nOcc<< 4) & WP;
        if (Temp) Movers |= (((Temp&MASK_L3) << 3) | ((Temp&MASK_L5) << 5)) & BK;
        Temp = ( ((nOcc&MASK_L3) << 3) | ((nOcc&MASK_L5) << 5) ) & WP;
        if (Temp) Movers |= (Temp << 4) & BK;
        }
    return Movers;
}

void SCheckerBoard::ConvertFromSqs( char Sq[] )
{
    WP = 0;
    BP = 0;
    K = 0;
    for (int i = 0; i < 32; i++)
        {
        switch (Sq[ BoardLoc32[i] ]) {
            case WPIECE: WP |= S[i]; break;
            case BPIECE: BP |= S[i]; break;
            case WKING:  K |= S[i]; WP |= S[i]; break;
            case BKING:  K |= S[i]; BP |= S[i]; break;
            }
        }
}
//
//  E N D    B I T B O A R D S
//

struct CMoveList
    {
    // FUNCTIONS
    void inline Clear( ) {nJumps = 0;  
                          nMoves = 0;
                          }
    void inline SetMove( SMove &Move, int src, int dst)
        {
        Move.SrcDst = (src)+(dst<<6) + (1<<12);
        }
    void inline AddJump( SMove &Move, int pathNum)
        {
        Move.cPath[pathNum] = 0;
        Moves[ ++nJumps ] = Move;
        }
    void inline AddMove (int src, int dst)
        {
        Moves[ ++nMoves ].SrcDst = (src)+(dst<<6);
        return;
        }

    void inline AddSqDir( char board[], int square, int &nSqMoves, int nPiece, int pathNum, const int DIR, const int OPP_PIECE );
    void inline CheckJumpDir( char board[], int square, const int DIR, const int nOppPiece);
    void FindMovesBlack( char board[], SCheckerBoard &C);
    void FindMovesWhite( char board[], SCheckerBoard &C);
    void FindJumpsBlack( char board[], int Movers);
    void FindJumpsWhite( char board[], int Movers);
    void FindNonJumpsBlack( char board[], int Movers);
    void FindNonJumpsWhite( char board[], int Movers);
    void inline FindSqJumpsBlack( char board[], int square, int nPiece, int pathNum, int nJumpSq);
    void inline FindSqJumpsWhite( char board[], int square, int nPiece, int pathNum, int nJumpSq);

    // DATA
    int nMoves, nJumps;
    SMove Moves[36];
    SMove m_JumpMove;
    };

CMoveList g_Movelist[ MAX_SEARCHDEPTH + 1];
SMove g_GameMoves[ MAX_GAMEMOVES ];

// ------------------------------ FUNCTIONS --------------------------------
// Check for a normal move in DIR, then if a normal move is not possible, check for a jump
void inline CMoveList::CheckJumpDir( char board[], int square, const int DIR, const int nOppPiece )
{
    if ( (board[square + DIR ]&nOppPiece) && board[square + 2*DIR ] == EMPTY) 
        {// A jump is possible, call FindSqJumps to detect double/triple/etc. jumps
         SetMove (m_JumpMove, square, square + 2*DIR);
         if (nOppPiece == WPIECE) FindSqJumpsBlack( board, square + 2*DIR, board[ square ], 0, square + DIR);
         if (nOppPiece == BPIECE) FindSqJumpsWhite( board, square + 2*DIR, board[ square ], 0, square + DIR);
        } 
}
// -------------------------------------------------
// Find the Moves available on board, and store them in Movelist
// -------------------------------------------------
void CMoveList::FindMovesBlack( char board[], SCheckerBoard &C )
   {
    UINT Movers = C.GetJumpersBlack();
    if ( Movers ) FindJumpsBlack( board, Movers );
    else {
        FindNonJumpsBlack( board, C.GetMoversBlack() );
        }
  }
void CMoveList::FindMovesWhite( char board[], SCheckerBoard &C )
    {
    UINT Movers = C.GetJumpersWhite( );

    
    if ( Movers ) {       
        FindJumpsWhite( board, Movers );
    }
    else {
        FindNonJumpsWhite( board, C.GetMoversWhite( ) );
        }
    }

void CMoveList::FindNonJumpsWhite( char board[], int Movers )
    {    
    Clear( );
    while (Movers) 
        {
        UINT sq = FindLowBit( Movers );
        Movers &= ~S[sq];
        int square = BoardLoc32[ sq ];

        if ( board[square-5 ] == EMPTY ) AddMove( square, square-5); 
        if ( board[square-4 ] == EMPTY ) AddMove( square, square-4); 

        if (board[ square ] == WKING  ) {
            if ( board[square+4 ] == EMPTY ) AddMove( square, square+4); 
            if ( board[square+5 ] == EMPTY ) AddMove( square, square+5); 
            }   
        }
    }

void CMoveList::FindNonJumpsBlack( char board[], int Movers )
   {
    Clear( );
    while (Movers) 
        {
        UINT sq = FindHighBit( Movers );
        Movers &= ~S[sq];
        int square = BoardLoc32[ sq ];

        if ( board[square+4 ] == EMPTY ) AddMove( square, square+4); 
        if ( board[square+5 ] == EMPTY ) AddMove( square, square+5); 

        if (board[ square ] == BKING  ) {
            if ( board[square-5 ] == EMPTY ) AddMove( square, square-5); 
            if ( board[square-4 ] == EMPTY ) AddMove( square, square-4); 
            }
        }
  }

// -------------------------------------------------
// These two functions only add jumps
// -------------------------------------------------
void CMoveList::FindJumpsBlack( char board[], int Movers )
   {
    Clear( );
    while (Movers) 
        {
        UINT sq = FindHighBit( Movers );
        Movers &= ~S[sq];
        int square = BoardLoc32[ sq ];

        CheckJumpDir( board, square, 4, WPIECE );
        CheckJumpDir( board, square, 5, WPIECE );

        if (board[ square ] == BKING  ) {
            CheckJumpDir( board, square, -5, WPIECE );
            CheckJumpDir( board, square, -4, WPIECE );
            }
        }
    nMoves = nJumps;
  }

void CMoveList::FindJumpsWhite( char board[], int Movers )
    {
    Clear( );
    while (Movers) 
        {
        UINT sq = FindLowBit( Movers );
        Movers &= ~ S[sq];
        int square = BoardLoc32[ sq ];

        CheckJumpDir( board, square, -5, BPIECE );
        CheckJumpDir( board, square, -4, BPIECE );

        if (board[ square ] == WKING  ) {
             CheckJumpDir( board, square, 4, BPIECE );
             CheckJumpDir( board, square, 5, BPIECE );
            }
        }
    nMoves = nJumps;
    }

// -------------
//  If a jump move was possible, we must make the jump then check to see if the same piece can jump again.
//  There might be multiple paths a piece can take on a double jump, these functions store each possible path as a move.
// -------------
void inline CMoveList::AddSqDir( char board[], int square, int &nSqMoves, int nPiece, int pathNum, const int DIR, const int OPP_PIECE )
    {
    if ( (board[square + DIR ]&OPP_PIECE) && board[square + 2*DIR ] == EMPTY ) 
        {m_JumpMove.cPath[ pathNum ] = square + 2*DIR; 
         nSqMoves++; 
         if (OPP_PIECE == WPIECE) FindSqJumpsBlack( board, square+2*DIR, nPiece, pathNum+1, square + DIR);
         if (OPP_PIECE == BPIECE) FindSqJumpsWhite( board, square+2*DIR, nPiece, pathNum+1, square + DIR);
        }
    }

void CMoveList::FindSqJumpsBlack( char board[], int square, int nPiece, int pathNum, int nJumpSq)
{
    int nSqMoves = 0;
    // Remove the jumped piece (until the end of this function), so we can't jump it again
    int nJumpPiece = board[ nJumpSq ];
    board[ nJumpSq ] = EMPTY;

    // Now see if a piece on this square has more jumps
    AddSqDir ( board, square, nSqMoves, nPiece, pathNum, 4, WPIECE );
    AddSqDir ( board, square, nSqMoves, nPiece, pathNum, 5, WPIECE );
    if ( nPiece == BKING) 
        {// If this piece is a king, it can also jump backwards 
        AddSqDir ( board, square, nSqMoves, nPiece, pathNum, -5, WPIECE );
        AddSqDir ( board, square, nSqMoves, nPiece, pathNum, -4, WPIECE );
        }
    if (nSqMoves == 0) AddJump( m_JumpMove, pathNum); // this is a leaf, so store the move
    // Put back the jumped piece
    board[ nJumpSq ] = nJumpPiece;
}

void CMoveList::FindSqJumpsWhite( char board[], int square, int nPiece, int pathNum, int nJumpSq)
{
    int nSqMoves = 0;
    int nJumpPiece = board[ nJumpSq ];
    board[ nJumpSq ] = EMPTY;

    AddSqDir ( board, square, nSqMoves, nPiece, pathNum, -4, BPIECE );
    AddSqDir ( board, square, nSqMoves, nPiece, pathNum, -5, BPIECE );  
    if ( nPiece == WKING) 
        {
        AddSqDir ( board, square, nSqMoves, nPiece, pathNum, 5, BPIECE );
        AddSqDir ( board, square, nSqMoves, nPiece, pathNum, 4, BPIECE );   
        }
    if (nSqMoves == 0) AddJump( m_JumpMove, pathNum );
    board[ nJumpSq ] = nJumpPiece;
}
