-- https://github.com/excessive/cpml/blob/master/modules/bvh.lua
-- https://github.com/benraziel/bvh-tree

--- BVH Tree
-- @module bvh

local vec3    = require("lib.vec3")
local EPSILON = 1e-6

local Node

local BVH     = {}
BVH.__index   = BVH

local function new(triangles, maxTrianglesPerNode)
    local tree                = setmetatable({}, BVH)

    tree._trianglesArray      = triangles
    tree._maxTrianglesPerNode = maxTrianglesPerNode or 10
    tree._bboxArray           = tree.calcBoundingBoxes(triangles)

    -- clone a helper array
    tree._bboxHelper          = {}
    for _, bbox in ipairs(tree._bboxArray) do
        table.insert(tree._bboxHelper, bbox)
    end

    -- create the root node, add all the triangles to it
    local triangleCount = #triangles / 9
    local extents = tree:calcExtents(1, triangleCount, EPSILON)
    tree._rootNode = Node(extents[1], extents[2], 1, triangleCount, 1)

    tree._nodes_to_split = { tree._rootNode }
    while #tree._nodes_to_split > 0 do
        local node = table.remove(tree._nodes_to_split)
        tree:splitNode(node)
    end
    return tree
end

local function aabb_vs_aabb(box1, box2)
    -- Check if there is no overlap in the x, y, and z axes
    if
        box1.max[1] < box2.min[1] or
        box1.min[1] > box2.max[1] then
        return false
    end

    if
        box1.max[2] < box2.min[2] or
        box1.min[2] > box2.max[2] then
        return false
    end

    if
        box1.max[3] < box2.min[3] or
        box1.min[3] > box2.max[3] then
        return false
    end

    -- If none of the above conditions are met, the boxes intersect
    return true
end

function BVH:intersectAABB(aabb)
    local nodesToIntersect             = { self._rootNode }
    local trianglesInIntersectingNodes = {} -- a list of nodes that intersect the ray (according to their bounding box)
    local intersectingTriangles        = {}

    -- go over the BVH tree, and extract the list of triangles that lie in nodes that intersect the box.
    -- note: these triangles may not intersect the box themselves
    while #nodesToIntersect > 0 do
        local node = table.remove(nodesToIntersect)

        local node_aabb = {
            min = node._extentsMin,
            max = node._extentsMax
        }

        if aabb_vs_aabb(aabb, node_aabb) then
            if node._node0 then
                table.insert(nodesToIntersect, node._node0)
            end

            if node._node1 then
                table.insert(nodesToIntersect, node._node1)
            end

            for i = node._startIndex, node._endIndex do
                table.insert(trianglesInIntersectingNodes, self._bboxArray[1 + (i - 1) * 7])
            end
        end
    end

    for i = 1, #trianglesInIntersectingNodes do
        local triIndex = trianglesInIntersectingNodes[i]

        table.insert(intersectingTriangles, {
            {
                self._trianglesArray[1 + (triIndex - 1) * 9],
                self._trianglesArray[1 + (triIndex - 1) * 9 + 1],
                self._trianglesArray[1 + (triIndex - 1) * 9 + 2]
            },

            {
                self._trianglesArray[1 + (triIndex - 1) * 9 + 3],
                self._trianglesArray[1 + (triIndex - 1) * 9 + 4],
                self._trianglesArray[1 + (triIndex - 1) * 9 + 5]
            },

            {
                self._trianglesArray[1 + (triIndex - 1) * 9 + 6],
                self._trianglesArray[1 + (triIndex - 1) * 9 + 7],
                self._trianglesArray[1 + (triIndex - 1) * 9 + 8]
            }
        })
    end

    return intersectingTriangles
end

local A = {}
local B = {}
local C = {}
local E1 = {}
local E2 = {}
local N = {}
local AO = {}
local DAO = {}

local function ray_triangle(triangle, origin, direction, backface_cull)
    A[1] = triangle[1]
    A[2] = triangle[2]
    A[3] = triangle[3]

    B[1] = triangle[4]
    B[2] = triangle[5]
    B[3] = triangle[6]

    C[1] = triangle[7]
    C[2] = triangle[8]
    C[3] = triangle[9]

    vec3.sub(B, A, 3, E1)
    vec3.sub(C, A, 3, E2)

    local h = vec3.cross(direction, E2)
    local a = vec3.dot(h, E1)

    -- if a is negative, ray hits the backface
    if backface_cull and a < 0 then
        return false
    end

    -- if a is too close to 0, ray does not intersect triangle
    if math.abs(a) <= EPSILON then
        return false
    end

    local f = 1 / a
    vec3.sub(origin, A, 3, AO)
    local u = vec3.dot(AO, h) * f

    -- ray does not intersect triangle
    if u < 0 or u > 1 then
        return false
    end

    local q = vec3.cross(AO, E1)
    local v = vec3.dot(direction, q) * f

    -- ray does not intersect triangle
    if v < 0 or u + v > 1 then
        return false
    end

    -- at this stage we can compute t to find out where
    -- the intersection point is on the line
    local t = vec3.dot(q, E2) * f

    -- return position of intersection and distance from ray origin
    if t >= EPSILON then
        return t
    end

    -- ray does not intersect triangle
    return false
end

function BVH:intersectRay(rayOrigin, rayDirection, backfaceCulling)
    local nodesToIntersect             = { self._rootNode }
    local trianglesInIntersectingNodes = {} -- a list of nodes that intersect the ray (according to their bounding box)
    local intersectingTriangles        = {}

    local invRayDirection              = {
        1 / rayDirection[1],
        1 / rayDirection[2],
        1 / rayDirection[3]
    }

    -- go over the BVH tree, and extract the list of triangles that lie in nodes that intersect the ray.
    -- note: these triangles may not intersect the ray themselves
    while #nodesToIntersect > 0 do
        local node = table.remove(nodesToIntersect)

        if BVH.intersectNodeBox(rayOrigin, invRayDirection, node) then
            if node._node0 then
                table.insert(nodesToIntersect, node._node0)
            end

            if node._node1 then
                table.insert(nodesToIntersect, node._node1)
            end

            for i = node._startIndex, node._endIndex do
                table.insert(trianglesInIntersectingNodes, self._bboxArray[1 + (i - 1) * 7])
            end
        end
    end

    -- go over the list of candidate triangles, and check each of them using ray triangle intersection
    local triangle = {}

    for i = 1, #trianglesInIntersectingNodes do
        local triIndex = trianglesInIntersectingNodes[i]

        -- print(triIndex, #self._trianglesArray)
        triangle[1] = self._trianglesArray[1 + (triIndex - 1) * 9]
        triangle[2] = self._trianglesArray[1 + (triIndex - 1) * 9 + 1]
        triangle[3] = self._trianglesArray[1 + (triIndex - 1) * 9 + 2]

        triangle[4] = self._trianglesArray[1 + (triIndex - 1) * 9 + 3]
        triangle[5] = self._trianglesArray[1 + (triIndex - 1) * 9 + 4]
        triangle[6] = self._trianglesArray[1 + (triIndex - 1) * 9 + 5]

        triangle[7] = self._trianglesArray[1 + (triIndex - 1) * 9 + 6]
        triangle[8] = self._trianglesArray[1 + (triIndex - 1) * 9 + 7]
        triangle[9] = self._trianglesArray[1 + (triIndex - 1) * 9 + 8]

        local t = ray_triangle(triangle, rayOrigin, rayDirection)

        if t then
            table.insert(intersectingTriangles, {
                { triangle[1], triangle[2], triangle[3] },
                { triangle[4], triangle[5], triangle[6] },
                { triangle[7], triangle[8], triangle[9] },

                t = t,
                p = vec3.add(rayOrigin, vec3.mul_val(rayDirection, t))
            })
        end
    end

    return intersectingTriangles
end

function BVH.calcBoundingBoxes(trianglesArray)
    local p1x, p1y, p1z
    local p2x, p2y, p2z
    local p3x, p3y, p3z
    local minX, minY, minZ
    local maxX, maxY, maxZ

    local bboxArray = {}

    for i = 1, #trianglesArray / 9 do
        p1x = trianglesArray[1 + (i - 1) * 9]
        p1y = trianglesArray[1 + (i - 1) * 9 + 1]
        p1z = trianglesArray[1 + (i - 1) * 9 + 2]
        p2x = trianglesArray[1 + (i - 1) * 9 + 3]
        p2y = trianglesArray[1 + (i - 1) * 9 + 4]
        p2z = trianglesArray[1 + (i - 1) * 9 + 5]
        p3x = trianglesArray[1 + (i - 1) * 9 + 6]
        p3y = trianglesArray[1 + (i - 1) * 9 + 7]
        p3z = trianglesArray[1 + (i - 1) * 9 + 8]

        minX = math.min(p1x, p2x, p3x)
        minY = math.min(p1y, p2y, p3y)
        minZ = math.min(p1z, p2z, p3z)
        maxX = math.max(p1x, p2x, p3x)
        maxY = math.max(p1y, p2y, p3y)
        maxZ = math.max(p1z, p2z, p3z)

        BVH.setBox(bboxArray, i, i, minX, minY, minZ, maxX, maxY, maxZ)
    end

    return bboxArray
end

function BVH:calcExtents(startIndex, endIndex, expandBy)
    expandBy = expandBy or 0

    if startIndex > endIndex then
        return { { 0, 0, 0 }, { 0, 0, 0 } }
    end

    local minX = math.huge
    local minY = math.huge
    local minZ = math.huge
    local maxX = -math.huge
    local maxY = -math.huge
    local maxZ = -math.huge

    for i = startIndex, endIndex do
        minX = math.min(self._bboxArray[1 + (i - 1) * 7 + 1], minX)
        minY = math.min(self._bboxArray[1 + (i - 1) * 7 + 2], minY)
        minZ = math.min(self._bboxArray[1 + (i - 1) * 7 + 3], minZ)
        maxX = math.max(self._bboxArray[1 + (i - 1) * 7 + 4], maxX)
        maxY = math.max(self._bboxArray[1 + (i - 1) * 7 + 5], maxY)
        maxZ = math.max(self._bboxArray[1 + (i - 1) * 7 + 6], maxZ)
    end

    return {
        { minX - expandBy, minY - expandBy, minZ - expandBy },
        { maxX + expandBy, maxY + expandBy, maxZ + expandBy }
    }
end

function BVH:splitNode(node)
    local num_elements = node:elementCount()
    if (num_elements <= self._maxTrianglesPerNode) or (num_elements <= 0) then
        return
    end

    local startIndex    = node._startIndex
    local endIndex      = node._endIndex

    local leftNode      = { {}, {}, {} }
    local rightNode     = { {}, {}, {} }
    local extentCenters = { node:centerX(), node:centerY(), node:centerZ() }

    local extentsLength = {
        node._extentsMax[1] - node._extentsMin[1],
        node._extentsMax[2] - node._extentsMin[2],
        node._extentsMax[3] - node._extentsMin[3]
    }

    local objectCenter  = {}
    for i = startIndex, endIndex do
        objectCenter[1] = (self._bboxArray[1 + (i - 1) * 7 + 1] + self._bboxArray[1 + (i - 1) * 7 + 4]) *
            0.5 -- center = (min + max) / 2
        objectCenter[2] = (self._bboxArray[1 + (i - 1) * 7 + 2] + self._bboxArray[1 + (i - 1) * 7 + 5]) *
            0.5 -- center = (min + max) / 2
        objectCenter[3] = (self._bboxArray[1 + (i - 1) * 7 + 3] + self._bboxArray[1 + (i - 1) * 7 + 6]) *
            0.5 -- center = (min + max) / 2

        for j = 1, 3 do
            if objectCenter[j] < extentCenters[j] then
                table.insert(leftNode[j], i)
            else
                table.insert(rightNode[j], i)
            end
        end
    end

    -- check if we couldn't split the node by any of the axes (x, y or z). halt
    -- here, dont try to split any more (cause it will always fail, and we'll
    -- enter an infinite loop
    local splitFailed = {
        #leftNode[1] == 0 or #rightNode[1] == 0,
        #leftNode[2] == 0 or #rightNode[2] == 0,
        #leftNode[3] == 0 or #rightNode[3] == 0
    }

    if splitFailed[1] and splitFailed[2] and splitFailed[3] then
        return
    end

    -- choose the longest split axis. if we can't split by it, choose next best one.
    local splitOrder = { 1, 2, 3 }
    table.sort(splitOrder, function(a, b)
        return extentsLength[a] > extentsLength[b]
    end)

    local leftElements
    local rightElements

    for i = 1, 3 do
        local candidateIndex = splitOrder[i]
        if not splitFailed[candidateIndex] then
            leftElements  = leftNode[candidateIndex]
            rightElements = rightNode[candidateIndex]
            break
        end
    end

    -- sort the elements in range (startIndex, endIndex) according to which node they should be at
    local node0Start = startIndex
    local node1Start = node0Start + #leftElements
    local node0End = node1Start - 1
    local node1End = endIndex
    local currElement

    local helperPos = node._startIndex
    local concatenatedElements = {}

    for _, element in ipairs(leftElements) do
        table.insert(concatenatedElements, element)
    end

    for _, element in ipairs(rightElements) do
        table.insert(concatenatedElements, element)
    end

    -- print(#leftElements, #rightElements, #concatenatedElements)

    for i = 1, #concatenatedElements do
        currElement = concatenatedElements[i]
        BVH.copyBox(self._bboxArray, currElement, self._bboxHelper, helperPos)
        helperPos = helperPos + 1
    end

    -- copy results back to main array
    for i = 1 + (node._startIndex - 1) * 7, node._endIndex * 7 do
        self._bboxArray[i] = self._bboxHelper[i]
    end

    -- create 2 new nodes for the node we just split, and add links to them from the parent node
    local node0Extents = self:calcExtents(node0Start, node0End, EPSILON)
    local node1Extents = self:calcExtents(node1Start, node1End, EPSILON)

    local node0 = Node(node0Extents[1], node0Extents[2], node0Start, node0End, node._level + 1)
    local node1 = Node(node1Extents[1], node1Extents[2], node1Start, node1End, node._level + 1)

    node._node0 = node0
    node._node1 = node1
    node:clearShapes()

    -- add new nodes to the split queue
    table.insert(self._nodes_to_split, node0)
    table.insert(self._nodes_to_split, node1)
end

function BVH._calcTValues(minVal, maxVal, rayOriginCoord, invdir)
    local res = { min = 0, max = 0 }

    if invdir >= 0 then
        res.min = (minVal - rayOriginCoord) * invdir
        res.max = (maxVal - rayOriginCoord) * invdir
    else
        res.min = (maxVal - rayOriginCoord) * invdir
        res.max = (minVal - rayOriginCoord) * invdir
    end

    return res
end

function BVH.intersectNodeBox(rayOrigin, invRayDirection, node)
    local t  = BVH._calcTValues(node._extentsMin[1], node._extentsMax[1], rayOrigin[1], invRayDirection[1])
    local ty = BVH._calcTValues(node._extentsMin[2], node._extentsMax[2], rayOrigin[2], invRayDirection[2])

    if t.min > ty.max or ty.min > t.max then
        return false
    end

    -- These lines also handle the case where tmin or tmax is NaN
    -- (result of 0 * Infinity). x !== x returns true if x is NaN
    if ty.min > t.min or t.min ~= t.min then
        t.min = ty.min
    end

    if ty.max < t.max or t.max ~= t.max then
        t.max = ty.max
    end

    local tz = BVH._calcTValues(node._extentsMin[3], node._extentsMax[3], rayOrigin[3], invRayDirection[3])

    if t.min > tz.max or tz.min > t.max then
        return false
    end

    if tz.min > t.min or t.min ~= t.min then
        t.min = tz.min
    end

    if tz.max < t.max or t.max ~= t.max then
        t.max = tz.max
    end

    --return point closest to the ray (positive side)
    if t.max < 0 then
        return false
    end

    return true
end

function BVH.setBox(bboxArray, pos, triangleId, minX, minY, minZ, maxX, maxY, maxZ)
    bboxArray[1 + (pos - 1) * 7] = triangleId
    bboxArray[1 + (pos - 1) * 7 + 1] = minX
    bboxArray[1 + (pos - 1) * 7 + 2] = minY
    bboxArray[1 + (pos - 1) * 7 + 3] = minZ
    bboxArray[1 + (pos - 1) * 7 + 4] = maxX
    bboxArray[1 + (pos - 1) * 7 + 5] = maxY
    bboxArray[1 + (pos - 1) * 7 + 6] = maxZ
end

function BVH.copyBox(sourceArray, sourcePos, destArray, destPos)
    destArray[1 + (destPos - 1) * 7] = sourceArray[1 + (sourcePos - 1) * 7]
    destArray[1 + (destPos - 1) * 7 + 1] = sourceArray[1 + (sourcePos - 1) * 7 + 1]
    destArray[1 + (destPos - 1) * 7 + 2] = sourceArray[1 + (sourcePos - 1) * 7 + 2]
    destArray[1 + (destPos - 1) * 7 + 3] = sourceArray[1 + (sourcePos - 1) * 7 + 3]
    destArray[1 + (destPos - 1) * 7 + 4] = sourceArray[1 + (sourcePos - 1) * 7 + 4]
    destArray[1 + (destPos - 1) * 7 + 5] = sourceArray[1 + (sourcePos - 1) * 7 + 5]
    destArray[1 + (destPos - 1) * 7 + 6] = sourceArray[1 + (sourcePos - 1) * 7 + 6]
end

function BVH.getBox(bboxArray, pos, outputBox)
    outputBox.triangleId = bboxArray[1 + (pos - 1) * 7]
    outputBox.minX       = bboxArray[1 + (pos - 1) * 7 + 1]
    outputBox.minY       = bboxArray[1 + (pos - 1) * 7 + 2]
    outputBox.minZ       = bboxArray[1 + (pos - 1) * 7 + 3]
    outputBox.maxX       = bboxArray[1 + (pos - 1) * 7 + 4]
    outputBox.maxY       = bboxArray[1 + (pos - 1) * 7 + 5]
    outputBox.maxZ       = bboxArray[1 + (pos - 1) * 7 + 6]
end

local BVHNode   = {}
BVHNode.__index = BVHNode

local function new_node(extentsMin, extentsMax, startIndex, endIndex, level)
    return setmetatable({
        _extentsMin = extentsMin,
        _extentsMax = extentsMax,
        _startIndex = startIndex,
        _endIndex   = endIndex,
        _level      = level
        --_node0    = nil
        --_node1    = nil
    }, BVHNode)
end

function BVHNode:elementCount()
    return (self._endIndex + 1) - self._startIndex
end

function BVHNode:centerX()
    return (self._extentsMin[1] + self._extentsMax[1]) * 0.5
end

function BVHNode:centerY()
    return (self._extentsMin[2] + self._extentsMax[2]) * 0.5
end

function BVHNode:centerZ()
    return (self._extentsMin[3] + self._extentsMax[3]) * 0.5
end

function BVHNode:clearShapes()
    self._startIndex = 0
    self._endIndex   = -1
end

function BVHNode.ngSphereRadius(extentsMin, extentsMax)
    local centerX = (extentsMin[1] + extentsMax[1]) * 0.5
    local centerY = (extentsMin[2] + extentsMax[2]) * 0.5
    local centerZ = (extentsMin[3] + extentsMax[3]) * 0.5

    local extentsMinDistSqr =
        (centerX - extentsMin[1]) * (centerX - extentsMin[1]) +
        (centerY - extentsMin[2]) * (centerY - extentsMin[2]) +
        (centerZ - extentsMin[3]) * (centerZ - extentsMin[3])

    local extentsMaxDistSqr =
        (centerX - extentsMax[1]) * (centerX - extentsMax[1]) +
        (centerY - extentsMax[2]) * (centerY - extentsMax[2]) +
        (centerZ - extentsMax[3]) * (centerZ - extentsMax[3])

    return math.sqrt(math.max(extentsMinDistSqr, extentsMaxDistSqr))
end

--[[

--- Draws node boundaries visually for debugging.
-- @param cube Cube model to draw
-- @param depth Used for recurcive calls to self method
function OctreeNode:draw_bounds(cube, depth)
	depth = depth or 0
	local tint = depth / 7 -- Will eventually get values > 1. Color rounds to 1 automatically

	love.graphics.setColor(tint * 255, 0, (1 - tint) * 255)
	local m = mat4()
		:translate(self.center)
		:scale(vec3(self.adjLength, self.adjLength, self.adjLength))

	love.graphics.updateMatrix("transform", m)
	love.graphics.setWireframe(true)
	love.graphics.draw(cube)
	love.graphics.setWireframe(false)

	for _, child in ipairs(self.children) do
		child:draw_bounds(cube, depth + 1)
	end

	love.graphics.setColor(255, 255, 255)
end

--- Draws the bounds of all objects in the tree visually for debugging.
-- @param cube Cube model to draw
-- @param filter a function returning true or false to determine visibility.
function OctreeNode:draw_objects(cube, filter)
	local tint = self.baseLength / 20
	love.graphics.setColor(0, (1 - tint) * 255, tint * 255, 63)

	for _, object in ipairs(self.objects) do
		if filter and filter(object.data) or not filter then
			local m = mat4()
				:translate(object.bounds.center)
				:scale(object.bounds.size)

			love.graphics.updateMatrix("transform", m)
			love.graphics.draw(cube)
		end
	end

	for _, child in ipairs(self.children) do
		child:draw_objects(cube, filter)
	end

	love.graphics.setColor(255, 255, 255)
end

--]]

Node = setmetatable({
    new = new_node
}, {
    __call = function(_, ...) return new_node(...) end
})

return setmetatable({
    new = new
}, {
    __call = function(_, ...) return new(...) end
})
