#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>

// 게임 상수 정의
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define BLOCK_SIZE 4
#define NEXT_BLOCK_X 20
#define NEXT_BLOCK_Y 4
#define FPS 5

// 블럭 모양 정의
typedef struct {
    int shape[BLOCK_SIZE][BLOCK_SIZE];
    int x;
    int y;
} Tetromino;

// 게임 변수 정의
int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};
Tetromino current_block;
Tetromino next_block;
Tetromino rotated_block;
int score = 0;
int level = 1;
int fall_speed = 1000000000 / FPS;  // 프레임 속도에 맞게 수정

// 게임 함수 선언
void init_game();
void draw_game();
void draw_block(int x, int y, int shape[BLOCK_SIZE][BLOCK_SIZE]);
void generate_random_block(Tetromino* block);
int is_collision(int x, int y, int shape[BLOCK_SIZE][BLOCK_SIZE]);
void merge_block();
void remove_completed_rows();
void update_score();
void update_level();

int main() 
{
    // ncurses 초기화
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    // 입력 비차단 모드 설정
    nodelay(stdscr, TRUE);
    // 랜덤 시드 초기화
    srand(time(NULL));

    // 게임 초기화
    init_game();
    //set difficulty
    int difficulty = 0;
    nodelay(stdscr,FALSE);
    while (difficulty < 1 || difficulty > 3) {
	    clear();
	    mvprintw(10, 10, "Choose difficulty level:");
	    mvprintw(12, 12, "1. Easy");
	    mvprintw(13, 12, "2. Normal");
	    mvprintw(14, 12, "3. Hard");
	    refresh();
	    difficulty = getch() - '0';
    }
    nodelay(stdscr,TRUE);
    // 낙하 속도 설정
    int drop_speed;
    switch (difficulty) {
	    case 1:
		    drop_speed = 1000000000 / FPS * 2; break;
	    case 2:
		    drop_speed = 1000000000 / FPS; break;
	    case 3:
		    drop_speed = 1000000000 / FPS / 2; break;
    }
    struct timespec tim;
    tim.tv_sec=0;
    tim.tv_nsec=drop_speed;

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC,&start_time);

    // 게임 루프
    while (1) 
    {
	struct timespec current_time;
	clock_gettime(CLOCK_MONOTONIC,&current_time);

	if((current_time.tv_sec-start_time.tv_sec)*1000000000 + (current_time.tv_nsec - start_time.tv_nsec) >= drop_speed){
        // 현재 블럭을 아래로 이동
        int new_y = current_block.y + 1;
        if (!is_collision(current_block.x, new_y, current_block.shape)) 
        {
            current_block.y = new_y;
        } 
        else 
        {
            merge_block();
            remove_completed_rows();
            update_score();
            update_level();
            
            
            
            // 새로운 현재 블럭 생성
            current_block = next_block;
            generate_random_block(&next_block);
	    //game over?
	    if(is_collision(current_block.x,current_block.y,current_block.shape)){
		    break;
	    }
        }
	clock_gettime(CLOCK_MONOTONIC,&start_time);
	}

        // 화면 갱신
        draw_game();

        // 사용자 입력 처리
        int ch = getch();
        switch (ch) 
        {
            case KEY_LEFT:
                if (!is_collision(current_block.x - 1, current_block.y, current_block.shape)) 
                {
                    current_block.x--;
                }
                break;
            case KEY_RIGHT:
                if (!is_collision(current_block.x + 1, current_block.y, current_block.shape)) 
                {
                    current_block.x++;
                }
                break;
            case KEY_DOWN:
                if (!is_collision(current_block.x, current_block.y + 1, current_block.shape)) 
                {
                    current_block.y++;
                }
                break;
            case KEY_UP:
                // 블록 회전
                rotated_block = current_block;
                for (int i = 0; i < BLOCK_SIZE; i++) 
                {
                    for (int j = 0; j < BLOCK_SIZE; j++) 
                    {
                        rotated_block.shape[i][j] = current_block.shape[BLOCK_SIZE - j - 1][i];
                    }
                }
                if (!is_collision(current_block.x, current_block.y, rotated_block.shape)) 
                {
                    current_block = rotated_block;
                }
                break;
            case 'q':
                // 게임 종료
                endwin();
                return 0;
        }

        // 60 FPS에 맞게 대기하지 않음
        timeout(1000 / FPS);
    }

    // 게임 종료 후 최종 점수 표시
    clear();
    mvprintw(10, 10, "Game Over");
    mvprintw(12, 10, "Final Score: %d", score);
    mvprintw(14, 10, "Final Level: %d", level);
    refresh();
    int end_timer=3;
    for(int i=end_timer;i>0;i--){
	    mvprintw(16,10,"Exiting in %d seconds...",i);
	    refresh();
	    sleep(1);
    }
    getch();

    // ncurses 정리
    endwin();
    return 0;
}

void init_game() 
{
    // 보드 초기화
    for (int i = 0; i < BOARD_HEIGHT; i++) 
    {
        for (int j = 0; j < BOARD_WIDTH; j++) 
        {
            board[i][j] = 0;
        }
    }
    // 현재 블럭 초기화
    current_block.x = BOARD_WIDTH / 2 - BLOCK_SIZE / 2;
    current_block.y = 0;
    generate_random_block(&current_block);
    // 다음 블럭 초기화
    generate_random_block(&next_block);
    // 점수 및 레벨 초기화
    score = 0;
    level = 1;
}

void draw_game() 
{
    clear();
    // 보드 그리기
    for (int i = 0; i < BOARD_HEIGHT; i++) 
    {
        for (int j = 0; j < BOARD_WIDTH; j++) 
        {
            if (board[i][j] == 0) 
            {
                // 빈 셀은 "."으로 출력
                mvprintw(i, j * 2, ".");
            } 
            else 
            {
                // 블럭은 "[]"으로 출력
                mvprintw(i, j * 2, "[]");
            }
        }
    }
    // 현재 블럭 그리기
    draw_block(current_block.x, current_block.y, current_block.shape);
    // 다음 블럭 그리기
    mvprintw(NEXT_BLOCK_Y, NEXT_BLOCK_X+10, "Next Block:");
    draw_block(NEXT_BLOCK_X, NEXT_BLOCK_Y + 2, next_block.shape);
    // 점수 및 레벨 표시
    mvprintw(2, NEXT_BLOCK_X, "Score: %d", score);
    mvprintw(4, NEXT_BLOCK_X, "Level: %d", level);
    // 화면 갱신
    refresh();
}

void draw_block(int x, int y, int shape[BLOCK_SIZE][BLOCK_SIZE]) 
{
    for (int i = 0; i < BLOCK_SIZE; i++) 
    {
        for (int j = 0; j < BLOCK_SIZE; j++) 
        {
            if (shape[i][j] != 0) 
            {
                // 블럭은 "[]"으로 출력
                mvprintw(y + i, (x + j) * 2, "[]");
            }
        }
    }
}

void generate_random_block(Tetromino* block) 
{
    // 랜덤한 블럭 모양 생성
    int random_shape = rand() % 7;
    switch (random_shape) {
        case 0:
            block->shape[0][0] = 1; 
            block->shape[0][1] = 1; 
            block->shape[0][2] = 1; 
            block->shape[0][3] = 1;
            block->shape[1][0] = 0; 
            block->shape[1][1] = 0; 
            block->shape[1][2] = 0; 
            block->shape[1][3] = 0;
            block->shape[2][0] = 0; 
            block->shape[2][1] = 0; 
            block->shape[2][2] = 0; 
            block->shape[2][3] = 0;
            block->shape[3][0] = 0; 
            block->shape[3][1] = 0; 
            block->shape[3][2] = 0; 
            block->shape[3][3] = 0;
            break;
        case 1:
            block->shape[0][0] = 1; 
            block->shape[0][1] = 1; 
            block->shape[0][2] = 0; 
            block->shape[0][3] = 0;
            block->shape[1][0] = 1; 
            block->shape[1][1] = 1; 
            block->shape[1][2] = 0; 
            block->shape[1][3] = 0;
            block->shape[2][0] = 0; 
            block->shape[2][1] = 0; 
            block->shape[2][2] = 0; 
            block->shape[2][3] = 0;
            block->shape[3][0] = 0; 
            block->shape[3][1] = 0; 
            block->shape[3][2] = 0; 
            block->shape[3][3] = 0;
            break;
        case 2:
            block->shape[0][0] = 0; 
            block->shape[0][1] = 1; 
            block->shape[0][2] = 0; 
            block->shape[0][3] = 0;
            block->shape[1][0] = 1; 
            block->shape[1][1] = 1; 
            block->shape[1][2] = 1; 
            block->shape[1][3] = 0;
            block->shape[2][0] = 0; 
            block->shape[2][1] = 0; 
            block->shape[2][2] = 0; 
            block->shape[2][3] = 0;
            block->shape[3][0] = 0; 
            block->shape[3][1] = 0; 
            block->shape[3][2] = 0; 
            block->shape[3][3] = 0;
            break;
        case 3:
            block->shape[0][0] = 0; 
            block->shape[0][1] = 1; 
            block->shape[0][2] = 1; 
            block->shape[0][3] = 0;
            block->shape[1][0] = 1; 
            block->shape[1][1] = 1; 
            block->shape[1][2] = 0; 
            block->shape[1][3] = 0;
            block->shape[2][0] = 0; 
            block->shape[2][1] = 0; 
            block->shape[2][2] = 0; 
            block->shape[2][3] = 0;
            block->shape[3][0] = 0; 
            block->shape[3][1] = 0; 
            block->shape[3][2] = 0; 
            block->shape[3][3] = 0;
            break;
        case 4:
            block->shape[0][0] = 1; 
            block->shape[0][1] = 0; 
            block->shape[0][2] = 0; 
            block->shape[0][3] = 0;
            block->shape[1][0] = 1; 
            block->shape[1][1] = 0; 
            block->shape[1][2] = 0; 
            block->shape[1][3] = 0;
            block->shape[2][0] = 1; 
            block->shape[2][1] = 0; 
            block->shape[2][2] = 0; 
            block->shape[2][3] = 0;
            block->shape[3][0] = 1; 
            block->shape[3][1] = 0; 
            block->shape[3][2] = 0; 
            block->shape[3][3] = 0;
            break;
        case 5:
            block->shape[0][0] = 1; 
            block->shape[0][1] = 0; 
            block->shape[0][2] = 0; 
            block->shape[0][3] = 0;
            block->shape[1][0] = 1; 
            block->shape[1][1] = 1; 
            block->shape[1][2] = 0; 
            block->shape[1][3] = 0;
            block->shape[2][0] = 0; 
            block->shape[2][1] = 1; 
            block->shape[2][2] = 0; 
            block->shape[2][3] = 0;
            block->shape[3][0] = 0; 
            block->shape[3][1] = 0; 
            block->shape[3][2] = 0; 
            block->shape[3][3] = 0;
            break;
        case 6:
            block->shape[0][0] = 0; 
            block->shape[0][1] = 1; 
            block->shape[0][2] = 0; 
            block->shape[0][3] = 0;
            block->shape[1][0] = 1; 
            block->shape[1][1] = 1; 
            block->shape[1][2] = 1; 
            block->shape[1][3] = 0;
            block->shape[2][0] = 0; 
            block->shape[2][1] = 0; 
            block->shape[2][2] = 0; 
            block->shape[2][3] = 0;
            block->shape[3][0] = 0; 
            block->shape[3][1] = 0; 
            block->shape[3][2] = 0; 
            block->shape[3][3] = 0;
            break;
    }
    // 블럭 초기 위치 설정
    block->x = BOARD_WIDTH / 2 - BLOCK_SIZE / 2;
    block->y = 0;
}

int is_collision(int x, int y, int shape[BLOCK_SIZE][BLOCK_SIZE]) {
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            // 블록의 현재 위치에서 x와 y를 더한 값이 보드의 경계를 벗어나는지 확인
            if (shape[i][j] != 0 &&
                (x + j < 0 || x + j >= BOARD_WIDTH || y + i >= BOARD_HEIGHT ||
                 (y + i < BOARD_HEIGHT && board[y + i][x + j] != 0))) {
                return 1; // 충돌이 발생하면 1을 반환
            }
        }
    }
    return 0; // 충돌이 없으면 0을 반환
}

void merge_block() 
{
    // 현재 블럭을 보드에 병합
    for (int i = 0; i < BLOCK_SIZE; i++) 
    {
        for (int j = 0; j < BLOCK_SIZE; j++) 
        {
            if (current_block.shape[i][j] != 0) 
            {
                board[current_block.y + i][current_block.x + j] = 1;
            }
        }
    }
}

void remove_completed_rows() 
{
    // 완성된 행 제거
    for (int i = BOARD_HEIGHT - 1; i >= 0; i--) 
    {
        int is_completed = 1;
        for (int j = 0; j < BOARD_WIDTH; j++) 
        {
            if (board[i][j] == 0) 
            {
                is_completed = 0;
                break;
            }
        }
        if (is_completed) 
        {
            // 현재 행 위의 모든 행을 한 칸씩 아래로 이동
            for (int k = i - 1; k >= 0; k--) 
            {
                for (int j = 0; j < BOARD_WIDTH; j++) 
                {
                    board[k + 1][j] = board[k][j];
                }
            }
            // 맨 위의 행을 0으로 초기화
            for (int j = 0; j < BOARD_WIDTH; j++) 
            {
                board[0][j] = 0;
            }
            // 점수 추가
            score += 10;
        }
    }
}

void update_score() 
{
    // 레벨에 따라 점수 증가
    score += level;
}

void update_level() 
{
    // 점수에 따라 레벨 증가
    level = score / 100 + 1;
}
