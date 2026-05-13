#include "player.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <memory>
#include <utility>
#include <vector>
#include <tuple>
#include <raylib.h>
#include "GameState.h"

Entity* PushAndInitializePlayer(GameState* gameState, VertexData* vertexData, VertexData* vertexDataEnd) {
	if (vertexDataEnd - vertexData < 3) {
		throw std::runtime_error("At least 3 vertices for the player");
	}
	Entity* returnPointer = (Entity*)PushEntity(gameState);

    std::cout << gameState->addedEntities;
	returnPointer->vertexData = vertexData;
    returnPointer->vertexDataEnd = vertexDataEnd;
	unsigned int* indices;
	unsigned int* indicesEnd;
	Triangulate2DPoints(returnPointer->vertexData, vertexDataEnd - vertexData, gameState, &indices, &indicesEnd);
	returnPointer->triangulationIndices = indices;
	returnPointer->triangulationIndicesEnd = indicesEnd;
    returnPointer->isPlayer = true;
    returnPointer->color = { 255, 0, 0, 255 };

	return returnPointer;
}

void PrintVector(Vector2 vec) {
    std::cout << "(X: " << vec.x << " ,Y: " << vec.y << " )" ;
}

void DrawPlayer(Entity* player) {
	for (int i = 0; i < player->triangulationIndicesEnd - player->triangulationIndices; i += 3) {
        Vector2 v1 = AddVectors((*(player->vertexData + player->triangulationIndices[i])).position, player->centerPosition);
        Vector2 v2 = AddVectors((*(player->vertexData + player->triangulationIndices[i+1])).position, player->centerPosition);
        Vector2 v3 = AddVectors((*(player->vertexData + player->triangulationIndices[i+2])).position, player->centerPosition);
        if (IsCounterClockwise(v1, v2, v3)) {
            DrawTriangle(v1, v2, v3, player->color);
        }
        else{
            DrawTriangle(v1, v3, v2, player->color);
        }
	}
}

bool IsCounterClockwise(Vector2 v1, Vector2 v2, Vector2 v3) {
    float crossProduct = (v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x);

    return crossProduct < 0.0f;
}

Vector2 AddVectors(Vector2& v1, Vector2& v2) {
    Vector2 newVector = { v1.x + v2.x, v1.y + v2.y };
    return newVector;
}

void MovePlayer(Entity* player, Vector2 mov) {
    player->centerPosition.x += mov.x;
    player->centerPosition.y += mov.y;
}

float square(float f1) {
    return f1 * f1;
}

float distance(Vector2 v1, Vector2 v2) {
    return sqrtf(square(v1.x - v2.x) + square(v1.y - v2.y));
}

int CalculateRelevantEntitiesFor(GameState* gameState, Entity* entity, Entity** relevanEntities) {
    Entity* entities = gameState->entities;
    Entity** relevantEntitiesEnd = relevanEntities;

    for (int i = 0; i < gameState->addedEntities; i++) {
        if (distance(entities[i].centerPosition, entity->centerPosition) < 300 && distance(entities[i].centerPosition, entity->centerPosition) != 0) {
            *(relevantEntitiesEnd++) = entities + i;
        }
    }
    return relevantEntitiesEnd - relevanEntities;
}

void CrossProduct(Vector3& vec1, Vector3& vec2, Vector3& crossProduct) {
    crossProduct.x = vec1.y * vec2.z - vec1.z * vec2.y;
    crossProduct.y = vec1.z * vec2.x - vec1.x * vec2.z;
    crossProduct.z = vec1.x * vec2.y - vec1.y * vec2.x;
}

void NormalVector(Vector3& vector, Vector3& normalVector) {
    Vector3 vector2;
    vector2.x = 0;
    vector2.y = 0;
    vector2.z = 1;

    CrossProduct(vector, vector2, normalVector);
}

double DotProduct(Vector3& vec1, Vector3& vec2) {
    return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
}

void CalculateAndApplyCollisionWithEntity(Entity* e1, Entity* e2) {
    double minOverlap = DBL_MAX;
    Vector3 overlapLine;

    int e1NumOfVertices = e1->vertexDataEnd - e1->vertexData;
    int e2NumOfVertices = e2->vertexDataEnd - e2->vertexData;
    for (int i = 0; i < e1NumOfVertices + (e2NumOfVertices) - 2; i++) {
        double player1Down = DBL_MAX, player1Up = -DBL_MAX;
        double player2Down = DBL_MAX, player2Up = -DBL_MAX;

        Vector3 vectorOfTwoVertices;
        Vector2 vertex1;
        Vector2 vertex2;
        if (i < e1NumOfVertices - 1) {
            vertex1 = AddVectors(e1->vertexData[i].position, e1->centerPosition);
            vertex2 = AddVectors(e1->vertexData[i + 1].position, e1->centerPosition);
        }
        else {
            int a = i - (e1NumOfVertices - 1);
            vertex1 = AddVectors(e2->vertexData[a].position, e1->centerPosition);
            vertex2 = AddVectors(e2->vertexData[a + 1].position, e1->centerPosition);
        }

        vectorOfTwoVertices.x = vertex2.x - vertex1.x;
        vectorOfTwoVertices.y = vertex2.y - vertex1.y;
        vectorOfTwoVertices.z = 0;

        Vector3 normal;
        NormalVector(vectorOfTwoVertices, normal);
        double normalLength = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);

        for (int j = 0; j < e1NumOfVertices; j++) {
            Vector3 posVector;
            //because we are taking the position relative to the center of e1 and vertex positions are relative to it
            //we dont subtract
            posVector.x = e1->vertexData[j].position.x;
            posVector.y = e1->vertexData[j].position.y;
            posVector.z = 0;

            double projectedLen = DotProduct(posVector, normal) / normalLength;

            if (projectedLen > player1Up) {
                player1Up = projectedLen;
            }
            else if (projectedLen < player1Down) {
                player1Down = projectedLen;
            }
        }

        for (int j = 0; j < e2NumOfVertices; j++) {
            Vector3 posVector;
            posVector.x = e2->vertexData[j].position.x + e2->centerPosition.x - e1->centerPosition.x;
            posVector.y = e2->vertexData[j].position.y + e2->centerPosition.y - e1->centerPosition.y;
            posVector.z = 0;

            double projectedLen = DotProduct(posVector, normal) / normalLength;
            if (projectedLen > player2Up) {
                player2Up = projectedLen;
            }
            else if (projectedLen < player2Down) {
                player2Down = projectedLen;
            }
        }
        /*
        std::cout << std::endl << "Normal: ";
        PrintVector3(normal);
        std::cout << "Players up and downs: " << std::endl;
        std::cout << "Player1Up: " << player1Up << " player1Down: " << player1Down << std::endl;
        std::cout << "Player2Up: " << player2Up << " player2Down: " << player2Down << std::endl;*/

        if (player1Up <= player2Down || player2Up <= player1Down) {
            minOverlap = 0;
        }
        else {
            double overlap = 0;
            if (player1Up <= player2Up) {
                if (player1Down >= player2Down) {
                    overlap = player1Up - player1Down;
                    if (overlap < minOverlap) {
                        minOverlap = overlap;
                        overlapLine.x = normal.x;
                        overlapLine.y = normal.y;
                        overlapLine.z = normal.z;
                    }
                }
                else {
                    overlap = player1Up - player2Down;
                    if (overlap < minOverlap) {
                        minOverlap = overlap;
                        overlapLine.x = normal.x;
                        overlapLine.y = normal.y;
                        overlapLine.z = normal.z;
                    }
                }
            }
            else {
                if (player1Down >= player2Down) {
                    overlap = player2Up - player1Down;
                    if (overlap < minOverlap) {
                        minOverlap = overlap;
                        overlapLine.x = normal.x;
                        overlapLine.y = normal.y;
                        overlapLine.z = normal.z;
                    }
                }
                else {
                    overlap = player2Up - player2Down;
                    if (overlap < minOverlap) {
                        minOverlap = overlap;
                        overlapLine.x = normal.x;
                        overlapLine.y = normal.y;
                        overlapLine.z = normal.z;
                    }
                }
            }
        }
    }
    if (minOverlap == DBL_MAX || minOverlap == 0) {
    }
    else {
        Vector3 otherPos = { e2->centerPosition.x, e2->centerPosition.y, 0 };
        Vector3 diffOfPositions;
        diffOfPositions.x = e1->centerPosition.x - otherPos.x;
        diffOfPositions.y = e1->centerPosition.y - otherPos.y;
        diffOfPositions.z = 0;

        if (DotProduct(diffOfPositions, overlapLine) < 0) {
            minOverlap *= -1;
        }

        float sin = overlapLine.y / sqrt(overlapLine.x * overlapLine.x + overlapLine.y * overlapLine.y);
        float cos = overlapLine.x / sqrt(overlapLine.x * overlapLine.x + overlapLine.y * overlapLine.y);

        MovePlayer(e1, { cos * (float)minOverlap, sin * (float)minOverlap });
    }
    
}

namespace delaunator {

//@see https://stackoverflow.com/questions/33333363/built-in-mod-vs-custom-mod-function-improve-the-performance-of-modulus-op/33333636#33333636
inline size_t fast_mod(const size_t i, const size_t c) {
    return i >= c ? i % c : i;
}

// Kahan and Babuska summation, Neumaier variant; accumulates less FP error
inline double sum(const std::vector<double>& x) {
    double sum = x[0];
    double err = 0.0;

    for (size_t i = 1; i < x.size(); i++) {
        const double k = x[i];
        const double m = sum + k;
        err += std::fabs(sum) >= std::fabs(k) ? sum - m + k : k - m + sum;
        sum = m;
    }
    return sum + err;
}

inline double dist(
    const double ax,
    const double ay,
    const double bx,
    const double by) {
    const double dx = ax - bx;
    const double dy = ay - by;
    return dx * dx + dy * dy;
}

inline double circumradius(
    const double ax,
    const double ay,
    const double bx,
    const double by,
    const double cx,
    const double cy) {
    const double dx = bx - ax;
    const double dy = by - ay;
    const double ex = cx - ax;
    const double ey = cy - ay;

    const double bl = dx * dx + dy * dy;
    const double cl = ex * ex + ey * ey;
    const double d = dx * ey - dy * ex;

    const double x = (ey * bl - dy * cl) * 0.5 / d;
    const double y = (dx * cl - ex * bl) * 0.5 / d;

    if ((bl > 0.0 || bl < 0.0) && (cl > 0.0 || cl < 0.0) && (d > 0.0 || d < 0.0)) {
        return x * x + y * y;
    } else {
        return std::numeric_limits<double>::max();
    }
}

inline bool orient(
    const double px,
    const double py,
    const double qx,
    const double qy,
    const double rx,
    const double ry) {
    return (qy - py) * (rx - qx) - (qx - px) * (ry - qy) < 0.0;
}

inline std::pair<double, double> circumcenter(
    const double ax,
    const double ay,
    const double bx,
    const double by,
    const double cx,
    const double cy) {
    const double dx = bx - ax;
    const double dy = by - ay;
    const double ex = cx - ax;
    const double ey = cy - ay;

    const double bl = dx * dx + dy * dy;
    const double cl = ex * ex + ey * ey;
    const double d = dx * ey - dy * ex;

    const double x = ax + (ey * bl - dy * cl) * 0.5 / d;
    const double y = ay + (dx * cl - ex * bl) * 0.5 / d;

    return std::make_pair(x, y);
}

struct compare {

    const VertexData* vertexData;
    double cx;
    double cy;

    bool operator()(std::size_t i, std::size_t j) {
        const double d1 = dist(vertexData[i].position.x, vertexData[i].position.y, cx, cy);
        const double d2 = dist(vertexData[j].position.x, vertexData[j].position.y, cx, cy);
        const double diff1 = d1 - d2;
        const double diff2 = vertexData[i].position.x - vertexData[j].position.x;
        const double diff3 = vertexData[i].position.y - vertexData[j].position.y;

        if (diff1 > 0.0 || diff1 < 0.0) {
            return diff1 < 0;
        } else if (diff2 > 0.0 || diff2 < 0.0) {
            return diff2 < 0;
        } else {
            return diff3 < 0;
        }
    }
};

inline bool in_circle(
    const double ax,
    const double ay,
    const double bx,
    const double by,
    const double cx,
    const double cy,
    const double px,
    const double py) {
    const double dx = ax - px;
    const double dy = ay - py;
    const double ex = bx - px;
    const double ey = by - py;
    const double fx = cx - px;
    const double fy = cy - py;

    const double ap = dx * dx + dy * dy;
    const double bp = ex * ex + ey * ey;
    const double cp = fx * fx + fy * fy;

    return (dx * (ey * cp - bp * fy) -
            dy * (ex * cp - bp * fx) +
            ap * (ex * fy - ey * fx)) < 0.0;
}

constexpr double EPSILON = std::numeric_limits<double>::epsilon();
constexpr std::size_t INVALID_INDEX = std::numeric_limits<std::size_t>::max();

inline bool check_pts_equal(double x1, double y1, double x2, double y2) {
    return std::fabs(x1 - x2) <= EPSILON &&
           std::fabs(y1 - y2) <= EPSILON;
}

// monotonically increases with real angle, but doesn't need expensive trigonometry
inline double pseudo_angle(const double dx, const double dy) {
    const double p = dx / (std::abs(dx) + std::abs(dy));
    return (dy > 0.0 ? 3.0 - p : 1.0 + p) / 4.0; // [0..1)
}

struct DelaunatorPoint {
    std::size_t i;
    double x;
    double y;
    std::size_t t;
    std::size_t prev;
    std::size_t next;
    bool removed;
};

class Delaunator {

public:
    GameState* gameState;
    const VertexData* vertexData;
    size_t numData;
    unsigned int* triangles;
    unsigned int* trianglesEnd;
    std::vector<std::size_t> halfedges;
    std::vector<std::size_t> hull_prev;
    std::vector<std::size_t> hull_next;
    std::vector<std::size_t> hull_tri;
    std::size_t hull_start;

    Delaunator(const VertexData* in_coords, size_t numData, GameState* state);

    double get_hull_area();

private:
    std::vector<std::size_t> m_hash;
    double m_center_x;
    double m_center_y;
    std::size_t m_hash_size;
    std::vector<std::size_t> m_edge_stack;

    std::size_t legalize(std::size_t a);
    std::size_t hash_key(double x, double y) const;
    std::size_t add_triangle(
        std::size_t i0,
        std::size_t i1,
        std::size_t i2,
        std::size_t a,
        std::size_t b,
        std::size_t c);
    void link(std::size_t a, std::size_t b);
};

Delaunator::Delaunator(const VertexData* in_data, size_t numData, GameState* state)
    : vertexData(in_data),
      numData(numData),
      gameState(state),
      triangles(),
      trianglesEnd(),
      halfedges(),
      hull_prev(),
      hull_next(),
      hull_tri(),
      hull_start(),
      m_hash(),
      m_center_x(),
      m_center_y(),
      m_hash_size(),
      m_edge_stack() {
    std::size_t n = numData;

    double max_x = std::numeric_limits<double>::min();
    double max_y = std::numeric_limits<double>::min();
    double min_x = std::numeric_limits<double>::max();
    double min_y = std::numeric_limits<double>::max();
    std::vector<std::size_t> ids;
    ids.reserve(n);

    for (std::size_t i = 0; i < n; i++) {
        const double x = vertexData[i].position.x;
        const double y = vertexData[i].position.y;

        if (x < min_x) min_x = x;
        if (y < min_y) min_y = y;
        if (x > max_x) max_x = x;
        if (y > max_y) max_y = y;

        ids.push_back(i);
    }
    const double cx = (min_x + max_x) / 2;
    const double cy = (min_y + max_y) / 2;
    double min_dist = std::numeric_limits<double>::max();

    std::size_t i0 = INVALID_INDEX;
    std::size_t i1 = INVALID_INDEX;
    std::size_t i2 = INVALID_INDEX;

    // pick a seed point close to the centroid
    for (std::size_t i = 0; i < n; i++) {
        const double d = dist(cx, cy, vertexData[i].position.x, vertexData[i].position.y);
        if (d < min_dist) {
            i0 = i;
            min_dist = d;
        }
    }

    const double i0x = vertexData[i0].position.x;
    const double i0y = vertexData[i0].position.y;

    min_dist = std::numeric_limits<double>::max();

    // find the point closest to the seed
    for (std::size_t i = 0; i < n; i++) {
        if (i == i0) continue;
        const double d = dist(i0x, i0y, vertexData[i].position.x, vertexData[i].position.y);
        if (d < min_dist && d > 0.0) {
            i1 = i;
            min_dist = d;
        }
    }

    double i1x = vertexData[i1].position.x;
    double i1y = vertexData[i1].position.y;

    double min_radius = std::numeric_limits<double>::max();

    // find the third point which forms the smallest circumcircle with the first two
    for (std::size_t i = 0; i < n; i++) {
        if (i == i0 || i == i1) continue;

        const double r = circumradius(
            i0x, i0y, i1x, i1y, vertexData[i].position.x, vertexData[i].position.y);

        if (r < min_radius) {
            i2 = i;
            min_radius = r;
        }
    }

    if (!(min_radius < std::numeric_limits<double>::max())) {
        throw std::runtime_error("not triangulation");
    }

    double i2x = vertexData[i2].position.x;
    double i2y = vertexData[i2].position.y;

    if (orient(i0x, i0y, i1x, i1y, i2x, i2y)) {
        std::swap(i1, i2);
        std::swap(i1x, i2x);
        std::swap(i1y, i2y);
    }

    std::tie(m_center_x, m_center_y) = circumcenter(i0x, i0y, i1x, i1y, i2x, i2y);

    // sort the points by distance from the seed triangle circumcenter
    std::sort(ids.begin(), ids.end(), compare{ vertexData, m_center_x, m_center_y });

    // initialize a hash table for storing edges of the advancing convex hull
    m_hash_size = static_cast<std::size_t>(std::llround(std::ceil(std::sqrt(n))));
    m_hash.resize(m_hash_size);
    std::fill(m_hash.begin(), m_hash.end(), INVALID_INDEX);

    // initialize arrays for tracking the edges of the advancing convex hull
    hull_prev.resize(n);
    hull_next.resize(n);
    hull_tri.resize(n);

    hull_start = i0;

    size_t hull_size = 3;

    hull_next[i0] = hull_prev[i2] = i1;
    hull_next[i1] = hull_prev[i0] = i2;
    hull_next[i2] = hull_prev[i1] = i0;

    hull_tri[i0] = 0;
    hull_tri[i1] = 1;
    hull_tri[i2] = 2;

    m_hash[hash_key(i0x, i0y)] = i0;
    m_hash[hash_key(i1x, i1y)] = i1;
    m_hash[hash_key(i2x, i2y)] = i2;

    std::size_t max_triangles = n < 3 ? 1 : 2 * n - 5;
    triangles = (unsigned int*)PushSize(gameState, (max_triangles * 3) * sizeof(unsigned int));
    trianglesEnd = triangles;
    halfedges.reserve(max_triangles * 3);
    add_triangle(i0, i1, i2, INVALID_INDEX, INVALID_INDEX, INVALID_INDEX);
    double xp = std::numeric_limits<double>::quiet_NaN();
    double yp = std::numeric_limits<double>::quiet_NaN();
    for (std::size_t k = 0; k < n; k++) {
        const std::size_t i = ids[k];
        const double x = vertexData[i].position.x;
        const double y = vertexData[i].position.y;

        // skip near-duplicate points
        if (k > 0 && check_pts_equal(x, y, xp, yp)) continue;
        xp = x;
        yp = y;

        // skip seed triangle points
        if (
            check_pts_equal(x, y, i0x, i0y) ||
            check_pts_equal(x, y, i1x, i1y) ||
            check_pts_equal(x, y, i2x, i2y)) continue;

        // find a visible edge on the convex hull using edge hash
        std::size_t start = 0;

        size_t key = hash_key(x, y);
        for (size_t j = 0; j < m_hash_size; j++) {
            start = m_hash[fast_mod(key + j, m_hash_size)];
            if (start != INVALID_INDEX && start != hull_next[start]) break;
        }

        start = hull_prev[start];
        size_t e = start;
        size_t q;

        while (q = hull_next[e], !orient(x, y, vertexData[e].position.x, vertexData[e].position.y, vertexData[q].position.x, vertexData[q].position.y)) { //TODO: does it works in a same way as in JS
            e = q;
            if (e == start) {
                e = INVALID_INDEX;
                break;
            }
        }

        if (e == INVALID_INDEX) continue; // likely a near-duplicate point; skip it

        // add the first triangle from the point
        std::size_t t = add_triangle(
            e,
            i,
            hull_next[e],
            INVALID_INDEX,
            INVALID_INDEX,
            hull_tri[e]);

        hull_tri[i] = legalize(t + 2);
        hull_tri[e] = t;
        hull_size++;

        // walk forward through the hull, adding more triangles and flipping recursively
        std::size_t next = hull_next[e];
        while (
            q = hull_next[next],
            orient(x, y, vertexData[next].position.x, vertexData[next].position.y, vertexData[q].position.x, vertexData[q].position.y)) {
            t = add_triangle(next, i, q, hull_tri[i], INVALID_INDEX, hull_tri[next]);
            hull_tri[i] = legalize(t + 2);
            hull_next[next] = next; // mark as removed
            hull_size--;
            next = q;
        }

        // walk backward from the other side, adding more triangles and flipping
        if (e == start) {
            while (
                q = hull_prev[e],
                orient(x, y, vertexData[q].position.x, vertexData[q].position.y, vertexData[e].position.x, vertexData[e].position.y)) {
                t = add_triangle(q, i, e, INVALID_INDEX, hull_tri[e], hull_tri[q]);
                legalize(t + 2);
                hull_tri[q] = t;
                hull_next[e] = e; // mark as removed
                hull_size--;
                e = q;
            }
        }

        // update the hull indices
        hull_prev[i] = e;
        hull_start = e;
        hull_prev[next] = i;
        hull_next[e] = i;
        hull_next[i] = next;

        m_hash[hash_key(x, y)] = i;
        m_hash[hash_key(vertexData[e].position.x, vertexData[e].position.y)] = e;
    }
}

double Delaunator::get_hull_area() {
    std::vector<double> hull_area;
    size_t e = hull_start;
    do {
        hull_area.push_back((vertexData[e].position.x - vertexData[hull_prev[e]].position.x) * (vertexData[e].position.y + vertexData[hull_prev[e]].position.y));
        e = hull_next[e];
    } while (e != hull_start);
    return sum(hull_area);
}

std::size_t Delaunator::legalize(std::size_t a) {
    std::size_t i = 0;
    std::size_t ar = 0;
    m_edge_stack.clear();

    // recursion eliminated with a fixed-size stack
    while (true) {
        const size_t b = halfedges[a];

        /* if the pair of triangles doesn't satisfy the Delaunay condition
        * (p1 is inside the circumcircle of [p0, pl, pr]), flip them,
        * then do the same check/flip recursively for the new pair of triangles
        *
        *           pl                    pl
        *          /||\                  /  \
        *       al/ || \bl            al/    \a
        *        /  ||  \              /      \
        *       /  a||b  \    flip    /___ar___\
        *     p0\   ||   /p1   =>   p0\---bl---/p1
        *        \  ||  /              \      /
        *       ar\ || /br             b\    /br
        *          \||/                  \  /
        *           pr                    pr
        */
        const size_t a0 = 3 * (a / 3);
        ar = a0 + (a + 2) % 3;

        if (b == INVALID_INDEX) {
            if (i > 0) {
                i--;
                a = m_edge_stack[i];
                continue;
            } else {
                //i = INVALID_INDEX;
                break;
            }
        }

        const size_t b0 = 3 * (b / 3);
        const size_t al = a0 + (a + 1) % 3;
        const size_t bl = b0 + (b + 2) % 3;

        const std::size_t p0 = triangles[ar];
        const std::size_t pr = triangles[a];
        const std::size_t pl = triangles[al];
        const std::size_t p1 = triangles[bl];

        const bool illegal = in_circle(
            vertexData[p0].position.x,
            vertexData[p0].position.y,
            vertexData[pr].position.x,
            vertexData[pr].position.y,
            vertexData[pl].position.x,
            vertexData[pl].position.y,
            vertexData[p1].position.x,
            vertexData[p1].position.y);

        if (illegal) {
            triangles[a] = p1;
            triangles[b] = p0;

            auto hbl = halfedges[bl];

            // edge swapped on the other side of the hull (rare); fix the halfedge reference
            if (hbl == INVALID_INDEX) {
                std::size_t e = hull_start;
                do {
                    if (hull_tri[e] == bl) {
                        hull_tri[e] = a;
                        break;
                    }
                    e = hull_next[e];
                } while (e != hull_start);
            }
            link(a, hbl);
            link(b, halfedges[ar]);
            link(ar, bl);
            std::size_t br = b0 + (b + 1) % 3;

            if (i < m_edge_stack.size()) {
                m_edge_stack[i] = br;
            } else {
                m_edge_stack.push_back(br);
            }
            i++;

        } else {
            if (i > 0) {
                i--;
                a = m_edge_stack[i];
                continue;
            } else {
                break;
            }
        }
    }
    return ar;
}

inline std::size_t Delaunator::hash_key(const double x, const double y) const {
    const double dx = x - m_center_x;
    const double dy = y - m_center_y;
    return fast_mod(
        static_cast<std::size_t>(std::llround(std::floor(pseudo_angle(dx, dy) * static_cast<double>(m_hash_size)))),
        m_hash_size);
}

std::size_t Delaunator::add_triangle(
    std::size_t i0,
    std::size_t i1,
    std::size_t i2,
    std::size_t a,
    std::size_t b,
    std::size_t c) {
    std::size_t t = trianglesEnd - triangles;
    *(trianglesEnd) = i0;
    trianglesEnd++;
    *(trianglesEnd) = i1;
    trianglesEnd++;
    *(trianglesEnd) = i2;
    trianglesEnd++;
    link(t, a);
    link(t + 1, b);
    link(t + 2, c);
    return t;
}

void Delaunator::link(const std::size_t a, const std::size_t b) {
    std::size_t s = halfedges.size();
    if (a == s) {
        halfedges.push_back(b);
    } else if (a < s) {
        halfedges[a] = b;
    } else {
        throw std::runtime_error("Cannot link edge");
    }
    if (b != INVALID_INDEX) {
        std::size_t s2 = halfedges.size();
        if (b == s2) {
            halfedges.push_back(a);
        } else if (b < s2) {
            halfedges[b] = a;
        } else {
            throw std::runtime_error("Cannot link edge");
        }
    }
}

} //namespace delaunator



void Triangulate2DPoints(VertexData* begin, size_t numOfPoints, GameState* gameState, unsigned int** indices, unsigned int** indicesEnd) {

	if (numOfPoints <= 3) {
		for (int i = 0; i < numOfPoints; i++) {
			**indicesEnd = i;
			(*indicesEnd)++;
		}
		return;
	}

	delaunator::Delaunator d(begin, numOfPoints, gameState);

	size_t numOfIndices = d.trianglesEnd - d.triangles;
	*indices = d.triangles;
	*indicesEnd = d.trianglesEnd;

	/*for (int i = 0; i < numOfIndices - 2; i += 3) {
		std::cout << "(X: " << (*(begin + indices[i])).x << ", Y: " << (*(begin + indices[i])).y << std::endl;
		std::cout << "(X: " << (*(begin + indices[i+1])).x << ", Y: " << (*(begin + indices[i+1])).y << std::endl;
		std::cout << "(X: " << (*(begin + indices[i+2])).x << ", Y: " << (*(begin + indices[i+2])).y << std::endl;
		std::cout << std::endl;
	}*/
};
