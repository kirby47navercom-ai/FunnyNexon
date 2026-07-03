# Direct2DGame

`termproject`의 Python/pico2d 리소스를 가져와 Direct2D C++로 이식한 Visual Studio / Rider 프로젝트.

## 열 파일

```text
C:\Users\kirby\OneDrive\바탕 화면\Direct2DGame\Direct2DGame.sln
```

## 코드 구조

```text
Direct2DGame\GameApp.*       Win32 창, 메시지 루프, 씬 전환
Direct2DGame\Renderer.*      Direct2D 렌더링, WIC PNG 로딩, 텍스트 출력
Direct2DGame\Scenes.*        타이틀, 홈, 상점, 전투, 결과, 엔딩 화면
Direct2DGame\Player.*        라모나 이동, 점프, 공격, 피격
Direct2DGame\Boss.*          스테이지별 보스, HP, 투사체
Direct2DGame\Input.*         키보드/마우스 입력 상태
Assets\                     termproject에서 가져온 이미지 리소스
```

## 조작

```text
A / D        이동
Shift        달리기
Space        점프
F + 마우스   제스처 공격
Esc          종료
```

CMake는 쓰지 않는다.
