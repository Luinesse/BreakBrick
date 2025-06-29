#include <windows.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#define PI 3.14                     // PI

using namespace std;

int gDegree = 270.0f;

// ������ ���Ȱ����� ��ȯ.
float deg2rad(float degree) {
    return degree * PI / 180.0f;
}

// ������ ȭ�� ����� ����� ����
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// �÷��̾� Ŭ���� ���� (������ ������ ������ �ִ�.)
class Player {
public:
    // ������ ������ġ�� ����ġ
    float startX;
    float startY;
    float destX;
    float destY;

    // ������ ������ ����
    float angle;
    float length;

public:
    Player(float sx, float sy, float l) : startX(sx), startY(sy), destX(0.0f), destY(0.0f), angle(0.0f), length(l) {
        // ������ ���ÿ� ���� �ʱ�ȭ
        updateDirection();
    }

    // ������ �������� ���� ������ ���� ������Ʈ
    void updateDirection() {
        angle = deg2rad(gDegree);
        destX = startX + cos(angle) * length;
        destY = startY + sin(angle) * length;
    }

    // ������ �׷��ش�.
    void draw(HDC hdc) {
        MoveToEx(hdc, startX, startY, NULL);
        LineTo(hdc, destX, destY);
    }
};

// ������ �ν� �� Ŭ���� ����
class Ball {
public:
    // ���� �߽���ǥ�� ������
    float centerX;
    float centerY;
    float radius;

    // ���� ���󰡴� ����, �ӵ�, ���⺤�� ���� (vx, vy)
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

    // ���� �����̵��� ��.
    void update() {
        centerX += vx;
        centerY += vy;

        // ���� ȭ��� �浹���� ��.
        // �ٴڿ� ��°� ���⼭ ó������ �ʴ´�.
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

    // ���� �׷��ش�.
    void draw(HDC hdc) {
        Ellipse(hdc, centerX - radius, centerY - radius, centerX + radius, centerY + radius);
    }

    // ���� ��������ϴ°� ?
    // ���� �ٴڿ� ��Ҵ����� �˻��Ѵ�.
    bool isDelete() {
        return centerY + radius > SCREEN_HEIGHT;
    }
};

// �μ����� ������ Ŭ���� ����
class Brick {
public:
    // ������ ������ ��ġ�� ����, ����
    float startX;
    float startY;
    float width;
    float height;

    // ������ ���� ��� �¾Ҵ��� ?
    int hitCnt = 0;

public:
    Brick(float sx, float sy, float w, float h) : startX(sx), startY(sy), width(w), height(h) {

    }

    // ������ �׷��ش�.
    void draw(HDC hdc) {
        Rectangle(hdc, startX, startY, startX + width, startY + height);
    }

    // ������ �μ����� �ϴ°� ?
    // 10�� ������ �μ������� ����
    bool isBroken() {
        return hitCnt >= 10;
    }
};

// ���� ������ ���� �����Ƿ� ���ͷ� ����
// ���� ������ �ٸ� ������ ������ ���� �����Ƿ� ����ũ �����ͷ� ����.
vector<unique_ptr<Ball>> vBall;
vector<unique_ptr<Brick>> vBricks;

// �÷��̾� ������ ȣ��
Player p(SCREEN_WIDTH / 2, SCREEN_HEIGHT, 100.0f);

// �Լ� ���� ����
// ������ �ʱ�ȭ�� ������ ���� �浹�˻� �Լ�
void InitBricks();
bool CheckCollision(const Ball& ball, const Brick& brick);

// ������ ���ν��� �Լ� ����
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// WinMain: ������ ���ø����̼��� ������
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {
    // ������ Ŭ���� ����
    const wchar_t CLASS_NAME[] = L"MyWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;            // ������ �޽��� ó�� �Լ�
    wc.hInstance = hInstance;            // ���� �ν��Ͻ�
    wc.lpszClassName = CLASS_NAME;       // Ŭ���� �̸�
    wc.hbrBackground = (HBRUSH)(WHITE_BRUSH); // ��� ��� ����


    RegisterClass(&wc); // ������ Ŭ���� ���

    // ������ ����
    HWND hWnd = CreateWindowEx(
        0,                              // Ȯ�� ��Ÿ��
        CLASS_NAME,                     // ������ Ŭ���� �̸�
        L"�ﰢ�Լ��� �̿��� ���� �׸���",               // Ÿ��Ʋ �ٿ� ǥ�õ� �̸�
        WS_OVERLAPPEDWINDOW,           // ������ ��Ÿ��
        CW_USEDEFAULT, CW_USEDEFAULT,  // ��ġ
        SCREEN_WIDTH, SCREEN_HEIGHT,                       // ũ��
        NULL, NULL, hInstance, NULL
    );

    // ������ �ʱ�ȭ�� ������ ȭ���� ��������� �����Ѵ�.
    InitBricks();

    if (hWnd == NULL) {
        return 0;
    }

    ShowWindow(hWnd, nCmdShow); // ������ ǥ��

    SetTimer(hWnd, 1, 33, NULL); // ȭ�� ������ ���� Ÿ�̸� ȣ��

    // �޽��� ����
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

// �޽��� ó�� �Լ�
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_KEYDOWN:
        // ����, ������ ����Ű�� ������ ���� ����.
        // SPACE Ű�� ���� �߻�.
        // �ߺ� �Է��� �ǵ��� if - else if �� �ƴ� if�θ� ����
       if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
           gDegree -= 5.0f;
           // ���� ������Ʈ
           p.updateDirection();
       }
       if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
           gDegree += 5.0f;
           // ���� ������Ʈ
           p.updateDirection();
       }
       if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
           // ���� ������Ʈ
           p.updateDirection();
           // �߻�� �Ѿ��� ���Ϳ� �����Ͽ� �����Ѵ�.
           vBall.push_back(make_unique<Ball>(p.destX, p.destY, 6.0f, gDegree));
       }

       // ȭ�� �ٽ� �׸��� ��û
        InvalidateRect(hWnd, NULL, TRUE);

        return 0;

    case WM_TIMER:
        // �����Ӹ��� ������Ʈ �� ����
        for (auto& ball : vBall) {
            // ���� ������
            ball->update();

            for (auto& brick : vBricks) {
                // ���� ������ �浹 �˻� ����
                if (CheckCollision(*ball, *brick)) {
                    // ������ �浹Ƚ�� ����, ���� Y�� ���⼺�� �ݴ�� �������ش�.
                    brick->hitCnt++;
                    ball->vy = -ball->vy;
                    // �ߺ� �浹�� �����ʵ��� Ż��
                    break;
                }
            }
        }
        // remove_if, erase, ���ٽ��� Ȱ���� �ݹ��Լ��� ���� ������ ��������
        // ���� ������� ������ �����Ѵٸ�, ���Ϳ��� �ش� ���� �����Ѵ�.
        vBall.erase(remove_if(vBall.begin(), vBall.end(), [](unique_ptr<Ball>& b) {return b->isDelete(); }), vBall.end());
        // ������ ������� ������ �����Ѵٸ�, ���Ϳ��� �ش� ������ �����Ѵ�.
        vBricks.erase(remove_if(vBricks.begin(), vBricks.end(), [](unique_ptr<Brick>& b) {return b->isBroken(); }), vBricks.end());

        // ȭ�� �ٽ� �׸��� ��û
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // ������ �ִ� ������ �о������ ���� ȭ�� ��ü�� ������� �ٽ� ��ĥ
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, (HBRUSH)(WHITE_BRUSH));

        // ���� �׷��ش�.
        for (auto& ball : vBall) {
            ball->draw(hdc);
        }

        // ������ �׷��ش�.
        for (auto& brick : vBricks) {
            brick->draw(hdc);
        }

        // �÷��̾� ���� ���� ������Ʈ
        p.updateDirection();
        // ������ �׷��ش�.
        p.draw(hdc);

        EndPaint(hWnd, &ps);

        return 0;
    }
    case WM_DESTROY: // �����찡 ���� ��
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void InitBricks() {
    // ������ ����, ���� ������ ������ ����, ���� ���̿� ���� ������ ����
    const int rows = 5;
    const int cols = 10;
    const float brickWidth = 60.0f;
    const float brickHeight = 20.0f;
    const float margin = 10.0f;

    // ������ ������ ��ġ
    float startX = (SCREEN_WIDTH - (cols * (brickWidth + margin) - margin)) / 2;
    float startY = 50.0f;

    // �ݺ����� ���� ������ ��ġ�� ���Ͽ� ���Ϳ� ����
    for (int i = 0; i < rows; i++) {
        for (int k = 0; k < cols; k++) {
            float x = startX + k * (brickWidth + margin);
            float y = startY + i * (brickHeight + margin);
            vBricks.push_back(make_unique<Brick>(x, y, brickWidth, brickHeight));
        }
    }
}

bool CheckCollision(const Ball& ball, const Brick& brick) {
    // AABB �˻縦 ���� �浹 �˻�
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