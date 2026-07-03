# Direct2DGame

`C:\Users\kirby\OneDrive\바탕 화면\git\termproject`의 Python/pico2d 게임을 Direct2D C++ 프로젝트로 이식한 솔루션이야.

## 실행 파일

```text
C:\Users\kirby\OneDrive\바탕 화면\Direct2DGame\Direct2DGame.sln
```

Visual Studio나 Rider에서 이 `.sln`을 열면 돼. CMake는 쓰지 않아.

## 코드 구조

```text
Direct2DGame\GameApp.*            Win32 창, 메시지 루프, 씬 전환
Direct2DGame\Renderer.*           Direct2D/WIC 이미지 출력, 텍스트, 도형
Direct2DGame\Scenes.*             로고, 타이틀, 홈, 상점, 준비, 전투, 결과, 엔딩
Direct2DGame\Player.*             원본 ramona.py 기반 이동/점프/회피/피격/공격 상태
Direct2DGame\GestureRecognizer.*  원본 QDollarRecognizer.py의 $Q 제스처 인식 알고리즘
Direct2DGame\GestureCanvas.*      원본 draw_gesture.py 기반 F키 제스처 캔버스
Direct2DGame\Patterns.*           원본 pattern.py의 패턴 이름/이미지/랜덤 선택
Direct2DGame\Stages.*             stage*_monster.py, ghost_boss.py, hellkitty.py, siho.py 역할
Assets\                          termproject에서 가져온 이미지/제스처/사운드 리소스
```

## 조작

```text
A / D        이동
Shift        달리기
Space        점프 / 더블 점프
A 또는 D 두 번 회피
F 누른 상태   제스처 캔버스 열기
마우스 드래그 현재 보이는 패턴 그리기
Esc          종료
```

전투 데미지는 원본처럼 “몬스터/보스가 들고 있는 패턴”과 “그린 제스처 인식 결과”가 같을 때만 들어가.
