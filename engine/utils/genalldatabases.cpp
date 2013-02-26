/*
    genalldatabases.cpp - run this program to create the endgame database files 2pc.cdb, 3pc.cdb, 4pc.cdb 
   
    This file is part of Samuel

    The code in this file comes from the guicheckers project.
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

//
// This function will generate a win/loss/draw database. It's actually not hard to generate such a database. 
// Doing a good job is the tough part, but since I only want small databases for now, this is at least works.
//

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>

using namespace std;

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
#define DOUBLEJUMP 2000
#define HUMAN 128
#define MAKEMOVE 129
#define SEARCHED 127
#define KING_VAL 33
#define PD 0

int g_bDatabases = 0;
int bCheckerBoard = 0;

// Convert between 32 square board and internal 44 square board
int BoardLocTo32[48] = { 0, 0, 0, 0, 0, 0, 1, 2, 3, 0, 4, 5, 6, 7, 8,  9, 10, 11, 0, 12, 13, 14, 15, 16, 17, 18, 19, 0, 20, 21, 22, 23, 24, 25, 26, 27, 0, 28, 29, 30, 31};
int BoardLoc32[32] = { 5, 6, 7, 8, 10, 11, 12, 13, 14, 15, 16, 17, 19, 20, 21, 22, 23, 24, 25, 26, 28, 29, 30, 31, 32, 33, 34, 35, 37, 38, 39, 40};

UINT MASK_L3, MASK_L5, MASK_R3, MASK_R5, MASK_TOP, MASK_BOTTOM, MASK_TRAPPED_W, MASK_TRAPPED_B, MASK_2ND, MASK_7TH;
UINT MASK_EDGES, MASK_2CORNER1, MASK_2CORNER2;
UINT S[32];
unsigned char aBitCount[65536];
unsigned char aLowBit[65536];
unsigned char aHighBit[65536];
int PC2[4*4] = { WKING, BKING, EMPTY, EMPTY,    WKING, BPIECE, EMPTY, EMPTY,
                 WPIECE, BKING, EMPTY, EMPTY,   WPIECE, BPIECE, EMPTY, EMPTY};
int PC3[4*12] = {WKING, WKING, BKING, EMPTY,    WKING, BKING, BKING, EMPTY,
                 WKING, WKING, BPIECE, EMPTY,   WPIECE, BKING, BKING, EMPTY, 
                 WKING, WPIECE, BKING, EMPTY,   WKING, BKING, BPIECE, EMPTY,
                 WPIECE, WPIECE, BKING, EMPTY,  WKING, BPIECE, BPIECE, EMPTY,
                 WKING, WPIECE, BPIECE, EMPTY,  WPIECE, BKING, BPIECE, EMPTY,
                 WPIECE, WPIECE, BPIECE, EMPTY, WPIECE, BPIECE, BPIECE, EMPTY};
int PC4[4*9] =  {WKING, WKING, BKING, BKING,    WKING, WPIECE, BKING, BKING ,   WKING, WKING, BKING, BPIECE,
                 WKING, WPIECE, BKING, BPIECE,  WPIECE, WPIECE, BKING, BKING,   WKING, WKING, BPIECE, BPIECE,
                 WPIECE, WPIECE, BKING, BPIECE, WKING, WPIECE, BPIECE, BPIECE,  WPIECE, WPIECE, BPIECE, BPIECE };
int ThreeIndex[12]= { 0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13};
int FourIndex[9] = { 0, 1, 9, 2, 3, 11, 4, 12, 5};
int FlipResult[9] = { 0, 2, 1, 3 };
const int SIZE2 = 32/4 * 32 * 2;
const int SIZE3 = SIZE2 * 32;
int SIZE4 = 0;
unsigned char ResultsTwo  [ 4 * SIZE2 ];
unsigned char ResultsThree[ 6 * SIZE3 ];
unsigned char *ResultsFour;
unsigned char *pResults;
const int KV = KING_VAL;
const int KBoard[64] = { PD, PD, PD, PD, PD,
                         KV-3,  KV+0,  KV+0,  KV+0, PD,
                            KV+0,  KV+1,  KV+1,  KV+0,
                         KV+1,  KV+3,  KV+3,  KV+2, PD,
                            KV+3,  KV+5,  KV+5,  KV+1,
                         KV+1,  KV+5,  KV+5,  KV+3, PD,
                            KV+2,  KV+3,  KV+3,  KV+1,
                         KV+0,  KV+1,  KV+1,  KV+0, PD,
                            KV+0,  KV+0,  KV+0, KV-3};
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
//UINT MASK_L3, MASK_L5, MASK_R3, MASK_R5;

struct SDatabase 
{
    int nStart, nSizeW, nSizeB;
    int Pieces[4];
    int IndW[32*32], IndB[32*32];

    int inline GetIndex( int nW, int nB, int stm )
    {
        return nStart + stm + 2*IndW[ nW ] + 2*nSizeW*IndB[ nB ];
    }
};
SDatabase FourPc[12];

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

struct CBoard
{
    void StartPosition( int bResetRep );
    void Clear();
    void SetFlags();
    int EvaluateBoard (int ahead, int alpha, int beta);
    int InDatabase() 
    {
        if (!g_bDatabases || nWhite > 2 || nBlack > 2 ) return 0;
        return 1;
    }
    void ToFen(char *sFEN);
    int FromFen(char *sFEN);
    int FromPDN(char *sPDN);
    int ToPDN(char *sPDN);

    int MakeMovePDN( int src, int dst);
    int DoMove( SMove &Move, int nType);
    void DoSingleJump( int src, int dst, int nPiece);
    void CheckKing( int src, int dst, int nPiece);
    void UpdateSqTable( int nPiece, int src, int dst );

    // Data
    char Sqs[48];
    SCheckerBoard C;
    short nPSq, eval;
    char nWhite, nBlack, SideToMove, extra;
    u_int64_t HashKey;
};

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
        switch (Sq[ BoardLoc32[i] ])
        {
            case WPIECE:                
                WP |= S[i];                
                break;
            case BPIECE:
                BP |= S[i];
                break;
            case WKING:
                K |= S[i];
                WP |= S[i];
                break;
            case BKING:
                K |= S[i];
                BP |= S[i];
                break;
        }
    }
}
//
//  E N D    B I T B O A R D S
//

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
// These functions only add jumps
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

// =================================================
//
//              TRANSPOSITION TABLE
//
// =================================================
unsigned char g_ucAge = 0;

struct TEntry
    {
    // FUNCTIONS
    public:
    void inline Read( unsigned long CheckSum, short alpha, short beta, int &bestmove, int &value, int depth, int ahead)
        {
        if (m_checksum == CheckSum ) //To be almost totally sure these are really the same position.
            {
            int tempVal;
            // Get the Value if the search was deep enough, and bounds usable
            if (m_depth >= depth)
               {
               if (abs(m_eval) > 1800) // This is a game ending value, must adjust it since it depends on the variable ahead
                    {
                    if (m_eval > 0) tempVal = m_eval - ahead + m_ahead;
                    if (m_eval < 0) tempVal = m_eval + ahead - m_ahead;
                    }
               else tempVal = m_eval;
               switch ( m_failtype )
                    {
                    case 0: value = tempVal;  // Exact value
                        break;
                    case 1: if (tempVal <= alpha) value = tempVal; // Alpha Bound (check to make sure it's usuable)
                        break;
                    case 2: if (tempVal >= beta) value = tempVal; //  Beta Bound (check to make sure it's usuable)
                        break;
                    }
               }
            // Otherwise take the best move from Transposition Table
            bestmove = m_bestmove;
            }
        }

    void inline Write( unsigned long CheckSum, short alpha, short beta, int &bestmove, int &value, int depth, int ahead)
        {
        if (m_age == g_ucAge && m_depth > depth && m_depth > 14) return; // Don't write over deeper entries from same search
        m_checksum = CheckSum;
        m_eval = value;
        m_ahead = ahead;
        m_depth = depth;
        m_bestmove = bestmove;
        m_age = g_ucAge;
        if (value <= alpha) m_failtype = 1;
        else if (value >= beta)  m_failtype = 2;
        else m_failtype = 0;
        }

    static void Create_HashFunction();
    static u_int64_t HashBoard( const CBoard &Board );

    // DATA
    private:
        unsigned long m_checksum;
        short m_eval;
        short m_bestmove;
        char m_depth;
        char m_failtype, m_ahead;
        unsigned char m_age;
    };

u_int64_t HashFunction[48][12], HashSTM;

// ------------------
//  Hash Board
//
//  HashBoard is called to get the hash value of the start board.
// ------------------

u_int64_t TEntry::HashBoard( const CBoard &Board )
    {
    u_int64_t CheckSum = 0;

    for (int index = 5; index <= 40; index++)
        {
        int nPiece =  Board.Sqs [ index ];
        if (nPiece != EMPTY) CheckSum ^= HashFunction [ index ][ nPiece ];
        }
    if (Board.SideToMove == BLACK)
        {
        CheckSum ^= HashSTM;
        }
    return CheckSum;
    }

// =================================================
//
//              BOARD FUNCTIONS
//
// =================================================
void CBoard::Clear( )
{
    int i;
    for (i = 0; i< 48; i++) Sqs [i] = EMPTY;
    for (i = 0; i < 5; i++) Sqs [i] = INVALID;
    for (i = 41; i <  48; i++) Sqs [i] = INVALID;
    Sqs[9] = INVALID; Sqs[18] = INVALID; Sqs[27] = INVALID; Sqs[36] = INVALID;
    SetFlags ();
}

void CBoard::SetFlags( )
    {
    nWhite = 0; nBlack = 0;
    nPSq = 0;
    for (int i = 5; i < 41; i++)
        {
        if ( (Sqs[i]&WHITE) && Sqs[i]<INVALID) {nWhite++;
            if (Sqs[i] == WKING) {nPSq += KBoard[i];}
            }
        if ( (Sqs[i]&BLACK) && Sqs[i]<INVALID) {nBlack++;
            if (Sqs[i] == BKING) {nPSq -= KBoard[i];}
            }
        }
    HashKey = TEntry::HashBoard( *this );
    C.ConvertFromSqs( Sqs );
    }

//
// MOVE EXECUTION
//
// ---------------------
//  Helper Functions for DoMove
// ---------------------
void inline CBoard::DoSingleJump( const int src, const int dst, const int nPiece )
{    
    int jumpedSq = (dst + src) >> 1;
    int jumpedPiece = Sqs[ jumpedSq ];
    if ( jumpedPiece == WPIECE ) {nWhite--; }
        else if ( jumpedPiece == BPIECE) {nBlack--; }
        else if ( jumpedPiece == BKING)  {nBlack--; nPSq += KBoard[jumpedSq]; }
        else if ( jumpedPiece == WKING)  {nWhite--; nPSq -= KBoard[jumpedSq]; }
    
    HashKey ^= HashFunction[ src ][ nPiece ] ^ HashFunction[ dst ][ nPiece ] ^ HashFunction[ jumpedSq ][ jumpedPiece ];
    
    // Update the board array
    Sqs[ dst ] = nPiece;
    Sqs[ jumpedSq ] = EMPTY;
    Sqs[ src ] = EMPTY;

    // Update the bitboards
    UINT BitMove = (S[ BoardLocTo32[src] ] | S[ BoardLocTo32[dst] ]);
    if (SideToMove == BLACK) {
        C.WP ^= BitMove;
        C.BP &= ~S[BoardLocTo32[jumpedSq]];
        C.K  &= ~S[BoardLocTo32[jumpedSq]];
    } else {
        C.BP ^= BitMove;
        C.WP &= ~S[BoardLocTo32[jumpedSq]];
        C.K  &= ~S[BoardLocTo32[jumpedSq]];
    }
    if (nPiece & KING) C.K ^= BitMove;    
}

// This function will test if a checker needs to be upgraded to a king, and upgrade if necessary
void inline CBoard::CheckKing (const int src, const int dst, int const nPiece)
{
    if (dst <= 8)
        {Sqs[ dst ] = WKING;
         nPSq += KBoard[dst];
         HashKey  ^= HashFunction[ dst ][ nPiece ] ^ HashFunction[ dst ][ WKING ];
         C.K |= S[BoardLocTo32[dst]];
        }
    if (dst >= 37)
        {Sqs[ dst ] = BKING;
         nPSq -= KBoard[dst];
         HashKey ^= HashFunction[ dst ][ nPiece ] ^ HashFunction[ dst ][ BKING ];
         C.K |= S[BoardLocTo32[dst]];
        }
}

void inline CBoard::UpdateSqTable( const int nPiece, const int src, const int dst )
{
    switch (nPiece) {
        case BKING: nPSq -= KBoard[ dst ] - KBoard[ src ]; return;
        case WKING: nPSq += KBoard[ dst ] - KBoard[ src ]; return;
    }
}

// ---------------------
//  Execute a Move
// ---------------------
int CBoard::DoMove (SMove &Move, int nType)
    {
    unsigned int src = (Move.SrcDst&63);
    unsigned int dst = (Move.SrcDst>>6)&63;
    unsigned int bJump = (Move.SrcDst>>12);
    unsigned int nPiece = Sqs[src];    

    SideToMove ^=3;
    HashKey ^= HashSTM; // Flip hash stm

    if (bJump == 0)
        {        
        // Update the board array
        Sqs[ dst ] = Sqs[ src ];
        Sqs[ src ] = EMPTY;

        // Update the bitboards
        UINT BitMove = (S[ BoardLocTo32[src] ] | S[ BoardLocTo32[dst] ]);
        if (SideToMove == BLACK) C.WP ^= BitMove; else C.BP ^= BitMove;
        if (nPiece & KING) C.K ^= BitMove;
        
        // Update hash values
        HashKey ^= HashFunction[ src ][ nPiece ] ^ HashFunction[ dst ][ nPiece ];

        UpdateSqTable( nPiece, src, dst );
       
        if (nPiece < KING)  {
             CheckKing( src, dst, nPiece );
             return 1;}
        return 0;
        }

    DoSingleJump ( src, dst, nPiece);
    if (nPiece < KING) CheckKing( src, dst, nPiece );
    
    // Double Jump?
    if (Move.cPath[0] == 0)
        {UpdateSqTable ( nPiece, src, dst );
         return 1;
        }
    if (nType == HUMAN) return DOUBLEJUMP;
    
    for (int i = 0; i < 8; i++)
        {        
        int nDst = Move.cPath[i];       
        if (nDst == 0) break;       

        if (nType == MAKEMOVE) // pause a bit on displaying double jumps
            {
            //DrawBoard ( *this );             
             // Sleep (300);             
             //sleep (1);
            }
        DoSingleJump( dst, nDst, nPiece);
        dst = nDst;
        }    
    if (nPiece < KING) CheckKing( src, dst, nPiece );
    else UpdateSqTable ( nPiece, src, dst );    
    return 1;
    }

int GetResult( unsigned char *pResults, int Index )
    {   
    return( pResults[ Index/4 ] >> ((Index&3)*2) ) & 3;
    }

void SetResult ( int Index, int Result )
{    
    pResults[ Index/4 ] ^= (GetResult(pResults, Index) << ((Index&3)*2));
    pResults[ Index/4 ] |= (Result << ((Index&3)*2));
}

void Compute2PieceIndices( int pData[], int &nSize, int nP[] )
{
int nSq1, nSq2;
int bSame = (nP[0] == nP[1]) ? 1: 0;
nSize = 0;

for (nSq1 = 0; nSq1 < 32; nSq1++)
    for (nSq2 = 0; nSq2 < 32; nSq2++)
    {
    if ((nSq2 < nSq1 && bSame) ) pData[ nSq1 + nSq2*32 ] = pData[ nSq2 + nSq1*32 ];
    else if (nSq1 == nSq2) pData[ nSq1 + nSq2*32 ] = 0;
    else if ((nSq1 < 4 && nP[0] == WPIECE) || (nSq2 < 4 && nP[1] == WPIECE) || (nSq1 > 27 && nP[0] == BPIECE) || (nSq2 > 27 && nP[1] == BPIECE))
        {pData[ nSq1 + nSq2*32 ] = 0;}
    else {pData[ nSq1 + nSq2*32 ] = nSize;
          nSize++;
         }
    }
}

int ComputeAllIndices( )
{
    int nStart = 0;
    for (int i = 0; i < 9; i++)
    {
        int n = FourIndex[i];
        if (n > 8) continue;

        // copy 4*sizeof(int) characters from &PC4[4*i] to &FourPc[n].Pieces[0] 
        memcpy( &FourPc[n].Pieces[0], &PC4[4*i] , 4*sizeof(int) );

        FourPc[n].nStart = nStart;
        Compute2PieceIndices( FourPc[n].IndW, FourPc[n].nSizeW, &FourPc[n].Pieces[0] );
        Compute2PieceIndices( FourPc[n].IndB, FourPc[n].nSizeB, &FourPc[n].Pieces[2] );
        nStart += FourPc[n].nSizeW * FourPc[n].nSizeB * 2;
    }
    SIZE4 = nStart/4+1;
    ResultsFour = (unsigned char*) malloc ( nStart/4 + 4  );
    memset (ResultsFour,  0, SIZE4);
    memset (ResultsThree, 0, 6*SIZE3);
    memset (ResultsTwo,   0, 4*SIZE2);
    return SIZE4;
}

int ComputeIndex( int WS[], int BS[], int &nPieces, int stm, int &bFlip)
{
    int Index, i;
    nPieces = 0;
    bFlip = 0;
    if (WS[0] == -1)
    {         
        nPieces = 1;
        return 1;
    }
    if (BS[0] == -1)
    {        
        nPieces = 1;
        return 2;
    }    
    
    int Sqs[4];
    Sqs[ nPieces++ ] = (WS[0]>>4); 
    WS[0] = (WS[0]&15);
    if (WS[1] >= 0) {Sqs[ nPieces++ ]=(WS[1]>>4); WS[1] = (WS[1]&15); }
    Sqs[ nPieces++ ] = (BS[0]>>4);
    BS[0] = (BS[0]&15);
    if (BS[1] >= 0) {Sqs[ nPieces++ ]=(BS[1]>>4); BS[1] = (BS[1]&15); }

    if (nPieces == 2)
        {
        for (i = 0; i < 4; i++)
            if (WS[0] == PC2[0 + i*4] && BS[0] == PC2[0 + i*4+1]) break;
        Index = Sqs[0] + Sqs[1]*32 + stm*1024 + i*(32*32*2);
        return Index;
        }
    if (nPieces == 3)
        {
        for (i = 0; i < 12; i++)
            if ((WS[0] == PC3[0 + i*4] && WS[1] == PC3[1 + i*4] && BS[0] == PC3[2 + i*4]) ||
               ( WS[0] == PC3[0 + i*4] && BS[0] == PC3[1 + i*4] && BS[1] == PC3[2 + i*4])) break;
        if ( ThreeIndex[i] < 8 ) Index = Sqs[0] + Sqs[1]*32 + Sqs[2]*32*32 + stm*32*32*32;
        else {Index = (31-Sqs[1]) + (31-Sqs[2])*32 + (31-Sqs[0])*32*32 + (stm^1)*32*32*32;
              bFlip = 1;
            }
        return Index + (ThreeIndex[i]&7) * (32*32*32*2);
        }
    if (nPieces == 4)
        {
        for (i = 0; i < 9; i++)
            if (WS[0] == PC4[0 + i*4] && WS[1] == PC4[1 + i*4] && BS[0] == PC4[2 + i*4] && BS[1] == PC4[3 + i*4]) break;
        if ( FourIndex[i]<8 ) 
            Index = FourPc[ FourIndex[i] ].GetIndex( Sqs[0]+Sqs[1]*32, Sqs[2]+Sqs[3]*32, stm );
        else {Index = FourPc[ FourIndex[i]&7 ].GetIndex( (31-Sqs[2]) + (31-Sqs[3])*32 , (31-Sqs[0]) + (31-Sqs[1])*32, stm^1 ); 
              bFlip = 1;
             }
        return Index;
        }
    return 0;
}

//
// Get Index From Board
// Should be 2 of a color at most
//
int GetIndexFromBoard( CBoard &Board, int &bFlip, int &nPieces )
{
    int WSqs[2], BSqs[2], Piece;
    int stm = Board.SideToMove - 1;

    WSqs[0] = -1; WSqs[1] = -1;
    BSqs[0] = -1; BSqs[1] = -1;    

    UINT WPieces = Board.C.WP;   

    while ( WPieces )
    {        
        UINT sq = FindLowBit( WPieces );
        WPieces &= ~S[sq];
        Piece = Board.Sqs[ BoardLoc32[sq] ];
        if (WSqs[0] == -1)        {
           
            WSqs[0] = Piece + sq*16; 
        }
        else if ( Board.C.K & S[sq] ) {WSqs[1] = WSqs[0]; WSqs[0] = Piece + sq*16; }
        else WSqs[1] = Piece + sq*16;
    }

    UINT BPieces = Board.C.BP;
    while ( BPieces ) {
        UINT sq = FindLowBit( BPieces );
        BPieces &= ~S[sq];
        Piece = Board.Sqs[ BoardLoc32[sq] ];
        if (BSqs[0]==-1) { BSqs[0] = Piece+ sq*16; }
        else if ( Board.C.K & S[sq] ) {BSqs[1] = BSqs[0]; BSqs[0] = Piece + sq*16; }
        else BSqs[1] = Piece + sq*16;
    }
    return ComputeIndex( WSqs, BSqs, nPieces, stm, bFlip);
}

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

//
// Return a Win/Loss/Draw value for the board
//
int QueryDatabase( CBoard &Board )
{
    int bFlip, nPieces, Result = 3;
    int Index = GetIndexFromBoard (Board, bFlip, nPieces);

    if (nPieces == 1) return Index;
    if (nPieces == 2) Result = GetResult( ResultsTwo  , Index );
    if (nPieces == 3) Result = GetResult( ResultsThree, Index );
    if (nPieces == 4) Result = GetResult( ResultsFour , Index );
    if (bFlip) return FlipResult[ Result ];
    return Result;
}

int inline TestBoard( CBoard &Board, int nPieces, int np1, int np2, int np3, int RIndex )
{
    CMoveList Moves;
    CBoard OldBoard = Board;
    int Result, nLosses, Win, Loss, Index, i, bFlip;
    int nTotalWins = 0;

    for (int stm = 0; stm < 2; stm++)
    {
        Board.SideToMove = stm+1;
        OldBoard.SideToMove = stm+1;

        // problem is that nPieces = 1 every time - bug here!
        if (nPieces == 1) continue;
        

        Index = GetIndexFromBoard( Board, bFlip, nPieces );
        if (nPieces == 2) pResults = ResultsTwo;
        else if (nPieces == 3) pResults = ResultsThree;  
        else if (nPieces == 4) pResults = ResultsFour;      
        if (nPieces == 3) 
            {Index = np1 + np2*32 + np3*32*32 + stm*32*32*32 + (RIndex) * (32*32*32*2);}
        
        if (GetResult( pResults, Index ) != 0) {nTotalWins++; continue;}        

        if (Board.SideToMove == WHITE)
        {
            Moves.FindMovesWhite( Board.Sqs, Board.C ); Win = 2; Loss = 1;
        }
        else
        {
            Moves.FindMovesBlack( Board.Sqs, Board.C ); Win = 1; Loss = 2;
        }
                
        if (Moves.nMoves == 0)
        {            
            SetResult( Index, Loss ); nTotalWins++;         // Can't move, so it's a loss
        } 
        else if (Moves.nMoves != 0)
        {
            nLosses = 0;
            // Check the moves. If a move wins this is a win, if they all lose it's a loss, otherwise it's a draw
            for (i = 1; i <= Moves.nMoves; i++)
            {
                Board.DoMove( Moves.Moves[i], SEARCHED);
                Result = QueryDatabase( Board );
                Board  = OldBoard;
                if ( Result == Win ) {SetResult(Index, Win); nTotalWins++; break;}
                if ( Result == Loss ) nLosses++;
            }
            if ( nLosses == Moves.nMoves ) 
            {
                SetResult (Index, Loss);
                nTotalWins++;
            }
        }
    }
    return nTotalWins;
}

void GenDatabase( int Piece1, int Piece2, int Piece3, int Piece4, int RIndex )
{
    CBoard Board;

    int nLastTotalWins = -1, nTotalWins = 0;
    int it = 0, nPieces = -1, bFlip;

    while (nTotalWins != nLastTotalWins) // Keep going until we reach a plateau
    {
        nLastTotalWins = nTotalWins;
        nTotalWins = 0;
        
        // Loop through every possible position
        for (int np1 = 0; np1 < 32; np1++)
          for (int np2 = 0; np2 < 32; np2++)
            for (int np3 = 0; np3 < 32; np3++)
            {
                
                for (int np4 = 0; np4 < 32; np4++)
                {
                    
                    // Skip Illegal Positions
                    if (np1 == np2) continue;
                    
                    if (Piece3 != EMPTY)
                        if (np1 == np3 || np2 == np3) continue;
                    if (Piece4 != EMPTY)
                        if (np4 == np3 || np4 == np2 || np4 == np1) continue;
                    
                    // Setup the board
                    Board.Clear( );
                    Board.Sqs[ BoardLoc32[np4] ] = Piece4;
                    Board.Sqs[ BoardLoc32[np3] ] = Piece3;
                    Board.Sqs[ BoardLoc32[np2] ] = Piece2;
                    Board.Sqs[ BoardLoc32[np1] ] = Piece1;
                    Board.C.ConvertFromSqs( Board.Sqs );
                   
                    // Skip illegal positions( checker on back row )
                    if (Board.C.WP & ~Board.C.K & (S[0]|S[1]|S[2]|S[3]) ) continue;
                   
                    if (Board.C.BP & ~Board.C.K & (S[28]|S[29]|S[30]|S[31]) ) continue;
                    
                
                    if (nPieces == -1)
                    {
                        GetIndexFromBoard( Board, bFlip, nPieces );
                        if (bFlip) return;
                    }
                   
                    // npieces is always 1!!!! wrong
                    nTotalWins += TestBoard( Board, nPieces, np1, np2, np3, RIndex );
                    

                    if (Piece4 == EMPTY) break;
                }
                if (Piece3 == EMPTY) break;
            }

        it++;
    }
}

void SaveAllDatabases()
{

    char xff = 255;

    FILE *FP = fopen ("2pc.cdb", "wb");
    fwrite (ResultsTwo , 4, SIZE2, FP );
    fputc (xff, FP);
    fclose (FP);

    FP = fopen ("3pc.cdb", "wb");
    fwrite (ResultsThree, 6, SIZE3, FP );
    fputc (xff, FP);
    fclose (FP);

    FP = fopen ("4pc.cdb", "wb");
    fwrite (ResultsFour, 1, SIZE4, FP );
    fputc (xff, FP);
    fclose (FP);
}

void TEntry::Create_HashFunction( )
{
    for (int i = 0; i <48; i++)
        for (int x = 0; x < 9; x++)
            {HashFunction [i][x]  = rand() + (rand()*256) + (rand()*65536);
             HashFunction [i][x] <<= 32;
             HashFunction [i][x]  += rand() + (rand()*256) + (rand()*65536);
            }
    HashSTM = HashFunction[0][0];
}

void GenAllDatabases( )
{     

    InitBitTables();
    ComputeAllIndices();            
    
    for (int i = 0; i < 4; i++)  GenDatabase( PC2[ i*4 ], PC2[ i*4+1 ], PC2[ i*4+2 ], PC2[ i*4+3 ], i );
    for (int i = 0; i < 12; i++) GenDatabase( PC3[ i*4 ], PC3[ i*4+1 ], PC3[ i*4+2 ], PC3[ i*4+3 ], ThreeIndex[i] );
    for (int i = 0; i < 9; i++)  GenDatabase( PC4[ i*4 ], PC4[ i*4+1 ], PC4[ i*4+2 ], PC4[ i*4+3 ], FourIndex[i] );

    SaveAllDatabases();    
    
}

int main()
{    
    GenAllDatabases();    
}

