#include "Scene.h"
#include "Ball.h"
#include "Camera.h"
#include "Background.h"
#include "../Utils/Timer.h"

int Scene_DoubleCapacity(Scene *scene);

Scene *Scene_New(Renderer *renderer, int max_connections, float maxDistance)
{
    Scene *scene = NULL;
    int capacity = 1 << 10;

    int width  = Renderer_GetWidth(renderer);
    int height = Renderer_GetHeight(renderer);

    scene = (Scene *)calloc(1, sizeof(Scene));
    if (!scene) goto ERROR_LABEL;

    scene->m_textures = Textures_New(renderer);
    if (!scene->m_textures) goto ERROR_LABEL;

    scene->m_camera = Camera_New(width, height);
    if (!scene->m_camera) goto ERROR_LABEL;

    scene->m_input = Input_New();
    if (!scene->m_input) goto ERROR_LABEL;

    scene->m_balls = (Ball *)calloc(capacity, sizeof(Ball));
    if (!scene->m_balls) goto ERROR_LABEL;

    scene->m_queries = calloc(max_connections, sizeof(BallQuery));

    scene->m_renderer = renderer;
    scene->m_ballCount = 0;
    scene->m_ballCapacity = capacity;
    scene->m_timeStep = 1.0f / 100.f;
    scene->m_maxBalls = max_connections;
    scene->m_maxDistance = maxDistance;

    // Création d'une scène minimale avec trois balles reliées
    Ball *ball1 = Scene_CreateBall(scene, Vec2_Set(-0.75f, 0.0f));
    Ball *ball2 = Scene_CreateBall(scene, Vec2_Set(+0.75f, 0.0f));
    Ball *ball3 = Scene_CreateBall(scene, Vec2_Set(0.0f, 1.299f));
    Ball *ball4= Scene_CreateBall(scene, Vec2_Set(2.77f, 1.299f));
    Ball *ball5= Scene_CreateBall(scene, Vec2_Set(3.77f, 2.299f));
    Ball_Connect(ball1, ball2, 1.5f);
    Ball_Connect(ball1, ball3, 1.5f);
    Ball_Connect(ball2, ball3, 1.5f);
    Ball_Connect(ball4, ball3, 1.5f);
    Ball_Connect(ball5, ball3, 1.5f);
    Ball_Connect(ball4, ball5, 1.5f);

    return scene;

ERROR_LABEL:
    printf("ERROR - Scene_New()\n");
    assert(false);
    Scene_Free(scene);
    return NULL;
}

void Scene_Free(Scene *scene)
{
    if (!scene) return;

    Camera_Free(scene->m_camera);
    Input_Free(scene->m_input);
    Textures_Free(scene->m_textures);

    if (scene->m_balls)
    {
        free(scene->m_balls);
    }


    memset(scene, 0, sizeof(Scene));
    free(scene);
}

Renderer *Scene_GetRenderer(Scene *scene)
{
    return scene->m_renderer;
}

Camera *Scene_GetCamera(Scene *scene)
{
    return scene->m_camera;
}

Input *Scene_GetInput(Scene *scene)
{
    return scene->m_input;
}

Vec2 Scene_GetMousePosition(Scene *scene)
{
    return scene->m_mousePos;
}

int Scene_DoubleCapacity(Scene *scene)
{
    Ball *newBalls = NULL;
    int newCapacity = scene->m_ballCapacity << 1;

    newBalls = (Ball *)realloc(scene->m_balls, newCapacity * sizeof(Ball));
    if (!newBalls) goto ERROR_LABEL;

    scene->m_balls = newBalls;
    scene->m_ballCapacity = newCapacity;

    return EXIT_SUCCESS;

ERROR_LABEL:
    printf("ERROR - Scene_DoubleCapacity()\n");
    return EXIT_FAILURE;
}

Ball *Scene_CreateBall(Scene *scene, Vec2 position)
{
    if (scene->m_ballCount >= scene->m_ballCapacity)
    {
        int exitStatus = Scene_DoubleCapacity(scene);
        if (exitStatus == EXIT_FAILURE) goto ERROR_LABEL;
    }

    Ball *ball = &scene->m_balls[scene->m_ballCount];
    scene->m_ballCount++;

    *ball = Ball_Set(position);

    return ball;

ERROR_LABEL:
    printf("ERROR - Scene_CreateBall()\n");
    return NULL;
}

void Scene_RemoveBall(Scene *scene, Ball *ball)
{
    int ballCount = Scene_GetBallCount(scene);
    Ball *balls = Scene_GetBalls(scene);
    int index = (int)(ball - balls);
    int springCount = 0;

    if (index < 0 || index >= ballCount)
        return;

    // Supprime les ressorts liés à la balle
    springCount = ball->springCount;
    for (int i = 0; i < springCount; ++i)
    {
        Ball_Deconnect(ball, ball->springs[i].other);
    }

    Ball *lastBall = &balls[ballCount - 1];
    if (ball != lastBall)
    {
        // Copie la dernière balle à la position de la balle à supprimer
        *ball = *lastBall;

        // Met à jour ses ressorts
        ball->springCount = 0;
        springCount = lastBall->springCount;
        for (int i = 0; i < springCount; ++i)
        {
            Ball *other = lastBall->springs[i].other;
            float length = lastBall->springs[i].length;
            Ball_Deconnect(lastBall, other);
            Ball_Connect(ball, other, length);
        }
    }

    // Supprime la dernière balle
    scene->m_ballCount--;
}

int Scene_GetBallCount(Scene *scene)
{
    return scene->m_ballCount;
}

Ball *Scene_GetBalls(Scene *scene)
{
    return scene->m_balls;
}

BallQuery Scene_GetNearestBall(Scene *scene, Vec2 position)
{
    BallQuery query = { 0 };
    Ball *balls = Scene_GetBalls(scene); 
    int ballCount = Scene_GetBallCount(scene);

    // query.ball is already NULL
    if (!ballCount) return query;

    int min = Vec2_Distance(balls[0].position, position);
    for (size_t i = 0; i < ballCount; i++) {
        if (Vec2_Distance(balls[i].position, position) < min) {
            query.ball = &balls[i];
            query.distance = Vec2_Distance(balls[i].position, position);
        }
    }

    return query;
}

float scalar_product(Vec2 a, Vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

void BubbleSortBalls(Ball* balls, int ballCount, Vec2 pos)
{
    if (1 == ballCount) {
        return;
    }

    for (size_t i = 0; i < ballCount - 1; i++) {
        if (Vec2_Distance(balls[i].position, pos) > Vec2_Distance(balls[i+1].position, pos)) {
            Ball saved = balls[i];
            balls[i] = balls[i+1];
            balls[i+1] = saved;
        }
    }

    BubbleSortBalls(balls, ballCount - 1, pos);
}

_Bool isValidLength(Scene* scene, Vec2 v1, Vec2 v2) {
    return Vec2_Distance(v1, v2) < scene->m_maxDistance ? true : false;
}

int Scene_GetNearestBalls(Scene *scene, Vec2 position, BallQuery *queries, int queryCount)
{
    Ball* saved = calloc(Scene_GetBallCount(scene), sizeof(Ball));
    Ball *balls = Scene_GetBalls(scene);
    int ballCount = Scene_GetBallCount(scene);

    if (!ballCount) return EXIT_FAILURE;

    memcpy(saved, balls, ballCount * sizeof(Ball));
    BubbleSortBalls(saved, ballCount, position);

    for (int i = 0; i < queryCount; i++) {
        for (int k = 0; k < ballCount; ++k) {
            printf("%f == %f, %f == %f\n", saved[i].position.x, balls[k].position.x, saved[i].position.y, balls[k].position.y);
            if (saved[i].position.x == balls[k].position.x && saved[i].position.y == balls[k].position.y) {
                queries[i].ball = &balls[k];
                queries[i].distance = Vec2_Distance(queries[i].ball->position, position);                
            }
        }

        printf("%p\n", queries[i].ball);
    }

    scene->m_validCount = ValidBalls(scene, scene->m_queries, scene->m_mousePos, 10);

    free(saved);
    return EXIT_SUCCESS;
}

int mayDeleteBall(Scene* scene, Vec2 pos)
{
    if (EXIT_FAILURE == Scene_GetNearestBalls(scene, pos, scene->m_queries, 1)) {
        exit(EXIT_FAILURE);
    }

    if (Vec2_Distance(scene->m_queries[0].ball->position, pos) < 0.2f) {
        for (int i = 0; i < scene->m_queries[0].ball->springCount; ++i) Ball_Deconnect(scene->m_queries[0].ball, scene->m_queries[0].ball->springs[i].other);
        Scene_RemoveBall(scene, scene->m_queries[0].ball);
    }

}

void Scene_FixedUpdate(Scene *scene, float timeStep)
{
    int ballCount = Scene_GetBallCount(scene);
    Ball *balls = Scene_GetBalls(scene);

    for (int i = 0; i < ballCount; i++)
    {
        Ball_UpdateVelocity(&balls[i], timeStep);
    }
    for (int i = 0; i < ballCount; i++)
    {
        Ball_UpdatePosition(&balls[i], timeStep);
    }
}

void print_ball(Ball* ball) {
    fprintf(stdout, "ball.position.x / y: %f / %f, ball.velocity.x / y: %f / %f, ball.friction: %f, mass: %f\n"
                    , ball->position.x, ball->position.y, ball->velocity.x, ball->velocity.y, ball->friction, ball->mass);
}

Spring* InSpring(Scene* scene, Vec2 pos)
{
    BallQuery query = {0};
    Scene_GetNearestBalls(scene, pos, &query, 1);

    for (size_t i = 0; i < query.ball->springCount; i++) {
        if (!scalar_product(Vec2_Sub(pos, query.ball->springs[i].other->position), query.ball->position)) {
            return &query.ball->springs[i];
        }
    }

    return (void* )-1;
}

int connect_n(Scene* scene, int n, int max_length) {
    int ballCount = Scene_GetBallCount(scene);
    Ball* balls = Scene_GetBalls(scene);
    _Bool is_connected = false;

    if (n > ballCount || n < 0 || n > ballCount) {
        return EXIT_FAILURE;
    }

    if (EXIT_FAILURE == Scene_GetNearestBalls(scene, scene->m_mousePos, scene->m_queries, n)) {
        perror("failed to connect balls :(");
        exit(EXIT_FAILURE);
    }

    Ball *ball_created = Scene_CreateBall(scene, scene->m_mousePos);

    puts("=======");

    for (size_t i = 0; i < n; i++) {
        print_ball(ball_created);
        print_ball(scene->m_queries[i].ball);
        if (scene->m_queries[i].distance < max_length) {
            Ball_Connect(ball_created, scene->m_queries[i].ball, 3.5 + 0.2 * Vec2_Distance(scene->m_mousePos, scene->m_queries[i].ball->position));
            is_connected = true;
        } else if (!is_connected) {
            Scene_RemoveBall(scene, ball_created);
        }
    }

    return EXIT_SUCCESS;
}

int ValidBalls(Scene* scene, BallQuery* queries, Vec2 pos, int n)
{
    for (size_t i = 0; i < n; i++) {
        if (!queries[i].ball || !isValidLength(scene, pos, queries[i].ball->position)) return i;
    }

    return n;
}

void Scene_UpdateGame(Scene *scene)
{
    Input *input = Scene_GetInput(scene);
    Camera *camera = Scene_GetCamera(scene);

    // Initialise les requêtes
    scene->m_validCount = 0;

    // Calcule la position de la souris et son déplacement
    Vec2 mousePos = Vec2_Set(0.0f, 0.0f);
    Vec2 mouseDelta = Vec2_Set(0.0f, 0.0f);
    Camera_ViewToWorld(camera, (float)input->mouseX, (float)input->mouseY, &mousePos);
    Camera_ViewToWorld(
        camera,
        (float)(input->mouseX + input->mouseDeltaX),
        (float)(input->mouseY + input->mouseDeltaY),
        &mouseDelta
    );
    mouseDelta = Vec2_Sub(mouseDelta, mousePos);
    scene->m_mousePos = mousePos;

    // Déplacement de la caméra
    if (input->mouseRDown)
    {
        Camera_Move(camera, Vec2_Scale(mouseDelta, -1.f));
        return;
    }

    memset(scene->m_queries, 0x0, sizeof(BallQuery) * scene->m_maxBalls);

    // click detected
    if (scene->m_input->mouseLPressed && scene->m_mousePos.y > 0.0f) {
        connect_n(scene, 3, scene->m_maxDistance);
    } else if (scene->m_input->mouseDPressed) {
        mayDeleteBall(scene, scene->m_mousePos);
    } else if (EXIT_FAILURE == Scene_GetNearestBalls(scene, scene->m_mousePos, scene->m_queries, 3)) {
        exit(-1);
    }
}

void Scene_Update(Scene *scene)
{
    float timeStep = scene->m_timeStep;

    // Met à jour les entrées de l'utilisateur
    Input_Update(scene->m_input);

    // Met à jour le moteur physique (pas de temps fixe)
    scene->m_accu += Timer_GetDelta(g_time);
    while (scene->m_accu >= timeStep)
    {
        Scene_FixedUpdate(scene, timeStep);
        scene->m_accu -= timeStep;
    }

    // Met à jour la caméra (déplacement)
    Camera_Update(scene->m_camera);

    Scene_Render(scene);

    // Met à jour le jeu
    Scene_UpdateGame(scene);
}

void Scene_RenderBalls(Scene *scene)
{
    int ballCount = Scene_GetBallCount(scene);
    Ball *balls = Scene_GetBalls(scene);
    for (int i = 0; i < ballCount; i++)
    {
        Ball *ball = &balls[i];
        int springCount = ball->springCount;
        for (int j = 0; j < springCount; j++)
        {
            // Supprime le flag
            ball->springs[j].flags &= ~SPRING_RENDERED;
        }
    }

    for (int i = 0; i < ballCount; i++)
    {
        Ball *ball = &balls[i];
        Vec2 start = Ball_GetPosition(ball);

        int springCount = ball->springCount;
        for (int j = 0; j < springCount; j++)
        {
            Spring *spring = ball->springs + j;
            if ((spring->flags & SPRING_RENDERED) != 0)
            {
                continue;
            }

            // Ajoute le flag
            spring->flags |= SPRING_RENDERED;

            // Affiche le ressort
            Vec2 end = Ball_GetPosition(spring->other);
            Ball_RenderSpring(start, end, scene, true);
        }
    }

    for (int i = 0; i < ballCount; i++)
    {
        // Affiche la balle
        Ball_Render(&balls[i], scene);
    }
}

void Scene_Render(Scene *scene)
{
    // Dessine le fond (avec parallax)
    Background_Render(scene);

    // Dessine le sol
    TileMap_Render(scene);

    if (scene->m_input->mouseRDown == false)
    {
        // Dessine les ressorts inactifs
        int validCount = scene->m_validCount;
        BallQuery *queries = scene->m_queries;
        printf("valdCount: %d\n", scene->m_validCount);
        for (int i = 0; i < validCount; ++i)
        {
            Vec2 start = Scene_GetMousePosition(scene);
            Vec2 end = Ball_GetPosition(queries[i].ball);

            Ball_RenderSpring(start, end, scene, false);
        }
    }

    // Dessine les balles (avec les ressorts actifs)
    Scene_RenderBalls(scene);
}