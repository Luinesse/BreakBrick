#include <windows.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#define PI 3.14                     // PI

using namespace std;

int gDegree = 270.0f;

// 각도를 라디안값으로 변환.
float deg2rad(float degree) {
    return degree * PI / 180.0f;
}

// 윈도우 화면 사이즈를 상수로 정의
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// 플레이어 클래스 선언 (직선의 정보를 가지고 있다.)
class Player {
public:
    // 직선의 시작위치와 끝위치
    float startX;
    float startY;
    float destX;
    float destY;

    // 직선의 각도와 길이
    float angle;
    float length;

public:
    Player(float sx, float sy, float l) : startX(sx), startY(sy), destX(0.0f), destY(0.0f), angle(0.0f), length(l) {
        // 생성과 동시에 방향 초기화
        updateDirection();
    }

    // 직선의 각도값을 통해 직선의 방향 업데이트
    void updateDirection() {
        angle = deg2rad(gDegree);
        destX = startX + cos(angle) * length;
        destY = startY + sin(angle) * length;
    }

    // 직선을 그려준다.
    void draw(HDC hdc) {
        MoveToEx(hdc, startX, startY, NULL);
        LineTo(hdc, destX, destY);
    }
};

// 벽돌을 부실 공 클래스 선언
class Ball {
public:
    // 공의 중심좌표와 반지름
    float centerX;
    float centerY;
    float radius;

    // 공이 날라가는 각도, 속도, 방향벡터 성분 (vx, vy)
    float angle;
    float speed;
    float vy;
    float vx;

public:
    Ball(float cx, float cy, float r, int d) : centerX(cx), centerY(cy), radius(r), angle(deg2rad(d)) {
        speed = 20.0f;
        vx = cos(angle) * speed;
        vy = sin(angle) * speed;
    }

    // 공이 움직이도록 함.
    void update() {
        centerX += vx;
        centerY += vy;

        // 공이 화면과 충돌했을 때.
        // 바닥에 닿는건 여기서 처리하지 않는다.
        if (centerY - radius <= 0) {
            vy = -vy;
            centerY = radius;
        }

        if (centerX - radius <= 0) {
            vx = -vx;
            centerX = radius;
        }

        if (centerX + radius >= SCREEN_WIDTH) {
            vx = -vx;
            centerX = SCREEN_WIDTH - radius;
        }
    }

    // 공을 그려준다.
    void draw(HDC hdc) {
        Ellipse(hdc, centerX - radius, centerY - radius, centerX + radius, centerY + radius);
    }

    // 공이 사라져야하는가 ?
    // 공이 바닥에 닿았는지를 검사한다.
    bool isDelete() {
        return centerY + radius > SCREEN_HEIGHT;
    }
};

// 부서야할 벽돌의 클래스 선언
class Brick {
public:
    // 벽돌이 시작할 위치와 가로, 세로
    float startX;
    float startY;
    float width;
    float height;

    // 벽돌이 현재 몇번 맞았는지 ?
    int hitCnt = 0;

public:
    Brick(float sx, float sy, float w, float h) : startX(sx), startY(sy), width(w), height(h) {

    }

    // 벽돌을 그려준다.
    void draw(HDC hdc) {
        Rectangle(hdc, startX, startY, startX + width, startY + height);
    }

    // 벽돌이 부서져야 하는가 ?
    // 10번 맞으면 부서지도록 설정
    bool isBroken() {
        return hitCnt >= 10;
    }
};

// 공과 벽돌은 수가 많으므로 벡터로 관리
// 공과 벽돌을 다른 곳에서 참조할 일이 없으므로 유니크 포인터로 관리.
vector<unique_ptr<Ball>> vBall;
vector<unique_ptr<Brick>> vBricks;

// 플레이어 생성자 호출
Player p(SCREEN_WIDTH / 2, SCREEN_HEIGHT, 100.0f);

// 함수 전방 선언
// 벽돌의 초기화와 벽돌과 공의 충돌검사 함수
void InitBricks();
bool CheckCollision(const Ball& ball, const Brick& brick);

// 윈도우 프로시저 함수 선언
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// WinMain: 윈도우 애플리케이션의 시작점
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {
    // 윈도우 클래스 정의
    const wchar_t CLASS_NAME[] = L"MyWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;            // 윈도우 메시지 처리 함수
    wc.hInstance = hInstance;            // 현재 인스턴스
    wc.lpszClassName = CLASS_NAME;       // 클래스 이름
    wc.hbrBackground = (HBRUSH)(WHITE_BRUSH); // 배경 흰색 설정


    RegisterClass(&wc); // 윈도우 클래스 등록

    // 윈도우 생성
    HWND hWnd = CreateWindowEx(
        0,                              // 확장 스타일
        CLASS_NAME,                     // 윈도우 클래스 이름
        L"삼각함수를 이용한 포신 그리기",               // 타이틀 바에 표시될 이름
        WS_OVERLAPPEDWINDOW,           // 윈도우 스타일
        CW_USEDEFAULT, CW_USEDEFAULT,  // 위치
        SCREEN_WIDTH, SCREEN_HEIGHT,                       // 크기
        NULL, NULL, hInstance, NULL
    );

    // 벽돌의 초기화는 윈도우 화면이 만들어지고 수행한다.
    InitBricks();

    if (hWnd == NULL) {
        return 0;
    }

    ShowWindow(hWnd, nCmdShow); // 윈도우 표시

    SetTimer(hWnd, 1, 33, NULL); // 화면 갱신을 위한 타이머 호출

    // 메시지 루프
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

// 메시지 처리 함수
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_KEYDOWN:
        // 왼쪽, 오른쪽 방향키로 포신의 각도 조절.
        // SPACE 키로 공을 발사.
        // 중복 입력이 되도록 if - else if 가 아닌 if로만 구성
       if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
           gDegree -= 5.0f;
           // 각도 업데이트
           p.updateDirection();
       }
       if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
           gDegree += 5.0f;
           // 각도 업데이트
           p.updateDirection();
       }
       if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
           // 각도 업데이트
           p.updateDirection();
           // 발사된 총알은 벡터에 삽입하여 관리한다.
           vBall.push_back(make_unique<Ball>(p.destX, p.destY, 6.0f, gDegree));
       }

       // 화면 다시 그리기 요청
        InvalidateRect(hWnd, NULL, TRUE);

        return 0;

    case WM_TIMER:
        // 프레임마다 업데이트 될 내용
        for (auto& ball : vBall) {
            // 공의 움직임
            ball->update();

            for (auto& brick : vBricks) {
                // 공과 벽돌의 충돌 검사 수행
                if (CheckCollision(*ball, *brick)) {
                    // 벽돌의 충돌횟수 증가, 공의 Y축 방향성분 반대로 뒤집어준다.
                    brick->hitCnt++;
                    ball->vy = -ball->vy;
                    // 중복 충돌이 되지않도록 탈출
                    break;
                }
            }
        }
        // remove_if, erase, 람다식을 활용한 콜백함수로 공과 벽돌의 삭제관리
        // 공이 사라지는 조건이 만족한다면, 벡터에서 해당 공을 삭제한다.
        vBall.erase(remove_if(vBall.begin(), vBall.end(), [](unique_ptr<Ball>& b) {return b->isDelete(); }), vBall.end());
        // 벽돌이 사라지는 조건이 만족한다면, 벡터에서 해당 벽돌을 삭제한다.
        vBricks.erase(remove_if(vBricks.begin(), vBricks.end(), [](unique_ptr<Brick>& b) {return b->isBroken(); }), vBricks.end());

        // 화면 다시 그리기 요청
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // 이전에 있던 내용을 밀어버리기 위해 화면 전체를 흰색으로 다시 색칠
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, (HBRUSH)(WHITE_BRUSH));

        // 공을 그려준다.
        for (auto& ball : vBall) {
            ball->draw(hdc);
        }

        // 벽돌을 그려준다.
        for (auto& brick : vBricks) {
            brick->draw(hdc);
        }

        // 플레이어 포신 방향 업데이트
        p.updateDirection();
        // 포신을 그려준다.
        p.draw(hdc);

        EndPaint(hWnd, &ps);

        return 0;
    }
    case WM_DESTROY: // 윈도우가 닫힐 때
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void InitBricks() {
    // 벽돌의 가로, 세로 개수와 벽돌의 가로, 세로 길이와 벽돌 사이의 간격
    const int rows = 5;
    const int cols = 10;
    const float brickWidth = 60.0f;
    const float brickHeight = 20.0f;
    const float margin = 10.0f;

    // 벽돌이 시작할 위치
    float startX = (SCREEN_WIDTH - (cols * (brickWidth + margin) - margin)) / 2;
    float startY = 50.0f;

    // 반복문을 통해 벽돌의 위치를 구하여 벡터에 삽입
    for (int i = 0; i < rows; i++) {
        for (int k = 0; k < cols; k++) {
            float x = startX + k * (brickWidth + margin);
            float y = startY + i * (brickHeight + margin);
            vBricks.push_back(make_unique<Brick>(x, y, brickWidth, brickHeight));
        }
    }
}

bool CheckCollision(const Ball& ball, const Brick& brick) {
    // AABB 검사를 통한 충돌 검사
    const float ballLeft = ball.centerX - ball.radius;
    const float ballRight = ball.centerX + ball.radius;
    const float ballTop = ball.centerY - ball.radius;
    const float ballBottom = ball.centerY + ball.radius;

    const float brickLeft = brick.startX;
    const float brickRight = brick.startX + brick.width;
    const float brickTop = brick.startY;
    const float brickBottom = brick.startY + brick.height;

    return !(ballRight < brickLeft || ballLeft > brickRight ||
        ballBottom < brickTop || ballTop > brickBottom);
}