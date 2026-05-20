/* Programing III課題：動物将棋
 * 機能：駒の移動、持ち駒の捕獲と再利用、成り、ファイルセーブ/ロード、ランダムAI
 * 技術：構造体、列挙体、ポインタ、バイナリファイルI/O、制御構文
 * マクロ：関数形式マクロABS、IN_RANGE を使用
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>

#define ROWS 4
#define COLS 3
#define SAVE_FILE "game_save.dat"
#define ABS(x) ((x)<0?-(x):(x))
#define IN_RANGE(x,y) ((x)>=0&&(x)<ROWS&&(y)>=0&&(y)<COLS)

typedef enum {EMPTY,LION,GIRAFFE,ELEPHANT,CHICK,CHICKEN} PieceType;
typedef enum {NONE,PLAYER,ENEMY} Owner;
typedef struct {PieceType type; Owner owner;} Piece;
typedef struct {Piece board[ROWS][COLS]; Piece captured_player[10],captured_enemy[10];
    int cp_count,ce_count; Owner turn,winner;} GameState;
typedef struct {int from_r,from_c,to_r,to_c;} Move;

void init_game(GameState *g); void print_board(const GameState *g);
char get_piece_char(PieceType t,Owner o); PieceType char_to_type(char c);
bool is_valid_move(const GameState *g,Move m); bool make_move(GameState *g,Move m);
bool is_checkmate(GameState *g); void switch_turn(GameState *g);
void generate_random_move(GameState *g,Move *m);
void save_game(const GameState *g); bool load_game(GameState *g);

int main(void) {
    GameState g; char s[256];
    srand((unsigned int)time(NULL));
    /* ゲーム開始：セーブデータ確認 */
    printf("=== 動物将棋 ===\n続きから(y/n): ");
    if(fgets(s,256,stdin)&&(*s=='y'||*s=='Y')){
        if(!load_game(&g)){printf("新規開始\n"); init_game(&g);}
    }else init_game(&g);
    printf("移動(r1 c1 r2 c2) 打つ(d h r c) 保存(save)\n");
    
    while(g.winner==NONE) {
        print_board(&g); int tr,tc,fr,fc; char ct,pc;
        if(g.turn==PLAYER) {
            printf("Player: "); if(!fgets(s,256,stdin))break;
            if(!strncmp(s,"save",4)){save_game(&g);return 0;}
            if(sscanf(s,"%c %c %d %d",&ct,&pc,&tr,&tc)==4&&(ct=='d'||ct=='D')){
                PieceType t=char_to_type(pc);int h=-1;
                for(int i=0;i<g.cp_count;i++)if(g.captured_player[i].type==t){h=i;break;}
                if(h!=-1&&make_move(&g,(Move){-1,h,tr,tc}))switch_turn(&g);
            }else if(sscanf(s,"%d %d %d %d",&fr,&fc,&tr,&tc)==4){
                if(make_move(&g,(Move){fr,fc,tr,tc}))switch_turn(&g);
            }
        }else{
            printf("Enemy... ");Move m;generate_random_move(&g,&m);
            if(m.from_r==-1)printf("DROP\n");
            else printf("(%d,%d)->(%d,%d)\n",m.from_r,m.from_c,m.to_r,m.to_c);
            make_move(&g,m);switch_turn(&g);
        }
        if(is_checkmate(&g))break;
    }
    print_board(&g);
    printf(g.winner==PLAYER?"Player WIN!\n":"Enemy WIN!\n");
    return 0;
}

void init_game(GameState *g){
    for(int r=0,c;r<ROWS;r++)for(c=0;c<COLS;c++)g->board[r][c]=(Piece){EMPTY,NONE};
    g->board[0][0]=(Piece){GIRAFFE,ENEMY};g->board[0][1]=(Piece){LION,ENEMY};
    g->board[0][2]=(Piece){ELEPHANT,ENEMY};g->board[1][1]=(Piece){CHICK,ENEMY};
    g->board[3][0]=(Piece){ELEPHANT,PLAYER};g->board[3][1]=(Piece){LION,PLAYER};
    g->board[3][2]=(Piece){GIRAFFE,PLAYER};g->board[2][1]=(Piece){CHICK,PLAYER};
    g->cp_count=g->ce_count=0;g->turn=PLAYER;g->winner=NONE;
}

char get_piece_char(PieceType t,Owner o){
    const char *s="lkzhn";if(t<1||t>5)return ' ';
    char c=s[t-1];return o==PLAYER?toupper(c):c;
}

PieceType char_to_type(char c){
    const char *s="lkzhn";for(int i=0;i<5;i++)if(tolower(s[i])==tolower(c))return i+1;
    return EMPTY;
}

void print_board(const GameState *g){
    printf("\n   0 1 2\n  -------\nE:");
    for(int i=0;i<g->ce_count;i++)printf("%c",get_piece_char(g->captured_enemy[i].type,ENEMY));
    printf("\n");
    for(int r=0;r<ROWS;r++){
        printf("%d|",r);
        for(int c=0;c<COLS;c++){Piece p=g->board[r][c];
            printf("%c",p.type==EMPTY?' ':get_piece_char(p.type,p.owner));}
        printf("|\n");
    }
    printf("P:");
    for(int i=0;i<g->cp_count;i++)printf("%c",get_piece_char(g->captured_player[i].type,PLAYER));
    printf("\n");
}

bool is_valid_move(const GameState *g,Move m){
    if(m.from_r==-1){
        if(!IN_RANGE(m.to_r,m.to_c)||g->board[m.to_r][m.to_c].type!=EMPTY)return false;
        int h=(g->turn==PLAYER)?g->cp_count:g->ce_count;
        return m.from_c>=0&&m.from_c<h;
    }
    if(!IN_RANGE(m.from_r,m.from_c)||!IN_RANGE(m.to_r,m.to_c))return false;
    Piece p=g->board[m.from_r][m.from_c],t=g->board[m.to_r][m.to_c];
    if(p.owner!=g->turn||t.owner==g->turn)return false;
    int dr=m.to_r-m.from_r,dc=m.to_c-m.from_c,adr=ABS(dr),adc=ABS(dc);
    int f=(g->turn==PLAYER)?-1:1;
    switch(p.type){
        case LION:return adr<=1&&adc<=1;
        case GIRAFFE:return(adr==1&&adc==0)||(adr==0&&adc==1);
        case ELEPHANT:return adr==1&&adc==1;
        case CHICK:return dc==0&&dr==f;
        case CHICKEN:return adr<=1&&adc<=1&&!(dr==-f&&adc==1);
    }
    return false;
}

bool make_move(GameState *g,Move m){
    if(!is_valid_move(g,m))return false;
    if(m.from_r==-1){
        Piece *h=(g->turn==PLAYER)?g->captured_player:g->captured_enemy;
        int *c=(g->turn==PLAYER)?&g->cp_count:&g->ce_count;
        g->board[m.to_r][m.to_c]=h[m.from_c];g->board[m.to_r][m.to_c].owner=g->turn;
        for(int i=m.from_c;i<*c-1;i++)h[i]=h[i+1];(*c)--;return true;
    }
    Piece t=g->board[m.to_r][m.to_c];
    if(t.type!=EMPTY){
        if(t.type==CHICKEN)t.type=CHICK;t.owner=g->turn;
        if(g->turn==PLAYER)g->captured_player[g->cp_count++]=t;
        else g->captured_enemy[g->ce_count++]=t;
    }
    g->board[m.to_r][m.to_c]=g->board[m.from_r][m.from_c];
    g->board[m.from_r][m.from_c]=(Piece){EMPTY,NONE};
    if(g->board[m.to_r][m.to_c].type==CHICK)
        if((g->turn==PLAYER&&m.to_r==0)||(g->turn==ENEMY&&m.to_r==3))
            g->board[m.to_r][m.to_c].type=CHICKEN;
    return true;
}

void switch_turn(GameState *g){g->turn=(g->turn==PLAYER)?ENEMY:PLAYER;}

bool is_checkmate(GameState *g){
    bool pl=false,el=false;
    for(int r=0,c;r<ROWS;r++)for(c=0;c<COLS;c++)
        if(g->board[r][c].type==LION){
            if(g->board[r][c].owner==PLAYER)pl=true;
            if(g->board[r][c].owner==ENEMY)el=true;
        }
    if(!pl){g->winner=ENEMY;return true;}
    if(!el){g->winner=PLAYER;return true;}return false;
}

void generate_random_move(GameState *g,Move *m){
    for(int a=0;a<1000;a++){
        int h=(g->turn==PLAYER)?g->cp_count:g->ce_count;
        if(rand()%2&&h>0){
            Move dm={-1,rand()%h,rand()%ROWS,rand()%COLS};
            if(is_valid_move(g,dm)){*m=dm;return;}
        }else{
            int r=rand()%ROWS,c=rand()%COLS;
            if(g->board[r][c].owner==g->turn)
                for(int dr=-1;dr<=1;dr++)
                    for(int dc=-1;dc<=1;dc++){
                        Move tm={r,c,r+dr,c+dc};
                        if(is_valid_move(g,tm)){*m=tm;return;}
                    }
        }
    }
    m->from_r=-2;
}

void save_game(const GameState *g){
    FILE *f=fopen(SAVE_FILE,"wb");
    if(f){fwrite(g,sizeof(GameState),1,f);fclose(f);}
}

bool load_game(GameState *g){
    FILE *f=fopen(SAVE_FILE,"rb");
    if(!f)return false;
    int r=fread(g,sizeof(GameState),1,f);fclose(f);return r==1;
}
