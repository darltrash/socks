---@meta

---@namespace
eng = {}

---@alias Color (integer[] | integer | nil)

---@alias Texture number[]

---@alias inputs
---| 'up'
---| 'down'
---| 'left'
---| 'right'
---| 'jump'
---| 'attack'
---| 'menu'

---@class Model
---@field data string
---@field length integer
---@field extra string?
---@field submeshes {[string]:integer[]}?
local Model

---@class Image
---@field w integer
---@field h integer
---@field pixels string
-- Pixels are encoded as RGBA
local Image

---@class Call
---@field disable boolean?
---@field model   table?
---@field tint    table?, integer?
---@field mesh    table
---@field texture Texture?
---@field range   table?
local Call

---@class AudioSettings
---@field looping  boolean?
---@field gain     number?
---@field area     number?
---@field pitch    number?
---@field position table?
---@field velocity table?
local AudioSettings

---@class Room
---@field init  fun(self: Room)?
---@field frame fun(self: Room, alpha: number, delta: number)?
---@field tick  fun(self: Room, timestep: number)?
---@field [any] any
-- Specifies a set of callbacks and a context for them.
local Room

---@param alpha number
---@param delta number
---@param focused boolean
-- Callback that runs every frame, usually VSync-alligned
eng.frame = function(alpha, delta, focused) end

---@param timestep number
-- Callback that runs each <timestep> seconds
eng.tick = function(timestep) end

-- Callback that runs when program closes
eng.close = function() end

---@param new_room Room
---@param ... any
-- Sets the current "room" being executed
eng.set_room = function(new_room, ...) end

---@type number
-- Contains the seconds since program startup, accumulates every frame
eng.timer = 0

---@type boolean
-- Contains whether game window is focused or not.
eng.focused = true

---@type boolean
-- Contains whether the game is running in debug mode
eng.debug = false

---@param seconds number
-- Waits N seconds (1/1000 precision)
eng.wait = function(seconds) end

---@alias Thread integer

---@param code string
---@return Thread
-- Offloads a task to the background
eng.offload = function(code) end

---@param thread Thread
---@return any?
---@return string?
-- Check state/return of offloaded task.
eng.fetch = function(thread) end

---@param name string
---@return string?
---@nodiscard
-- Reads a file off <name> in the game's package
eng.read = function(name) end

---@param name string
---@return Model
---@nodiscard
-- Loads a Model.
eng.load_model = function(name) end

---@param name string
---@return Image
---@nodiscard
-- Loads an image type, not a texture, mind you!
eng.load_image = function(name) end

-- Reloads the game textures, honestly a hack
eng.reload_tex = function() end

---@param w integer
---@param h integer
---@param force_ratio boolean?
-- Sets the video mode of the game (desired resolution, and whether to have adaptive ratios or not)
eng.videomode = function(w, h, force_ratio) end

---@param from table
---@param to table
---@param up table
-- Sets the camera
eng.camera = function(from, to, up) end

---@param call Call
-- Renders a 3D call!
eng.render = function(call) end

---@param call Call
-- Renders a 2D call, usually above everything else
eng.draw = function(call) end

---@param position table
---@param color Color
-- Pushes a light into the stack, color length equals intensity equals area
eng.light = function(position, color) end

---@param distance number
---@param clear_color Color
-- Sets how far away is too far, usually for fog!
eng.far = function(distance, clear_color) end

---@param enable boolean
-- Whether to enable dithering or not, I can't remember if I actually finished implementing this
eng.dithering = function(enable) end

---@param color Color
-- Sets the base lighting, or the ambient color
eng.ambient = function(color) end

---@param snap number
-- Sets the snapping amount (PS1 graphics emulation)
eng.snapping = function(snap) end

---@param texture Texture
---@param x number
---@param y number
---@param color Color?
---@param scale_x number?
---@param scale_y number?
---@see eng.draw
-- Renders a quad
eng.quad = function(texture, x, y, color, scale_x, scale_y) end

---@param x number
---@param y number
---@param w number
---@param h number
---@param color Color
---@see eng.draw
---@see eng.quad
-- Renders a rectangle
eng.rect = function(x, y, w, h, color) end

---@return number
---@return number
---@return boolean
eng.direction = function() end

---@param key inputs
---@return number?
---@nodiscard
-- Returns how many ticks has a key been pressed for
-- If it hasn't been pressed yet, it simply will return nil
-- > NOTE: UPDATES ONCE EVERY TICK, NOT EVERY FRAME
eng.input = function(key) end

---@return {[inputs]: string[]}
---@nodiscard
eng.current_bind = function() end

---@return string
-- Returns the accumulated text buffer
-- > NOTE: UPDATES ONCE EVERY TICK, NOT EVERY FRAME
eng.text = function() end

---@param text string
---@see eng.debug
-- Renders some basic VGA-esque text on top of EVERYTHING else,
-- useful for specific debugging cases!
-- > NOTE: ONLY RENDERS IF eng.debug IS TRUE
eng.log = function(text) end

---@param button integer
---@return boolean
---@nodiscard
-- Returns whether a mouse button is currently pressed
eng.mouse_down = function(button) end

---@return number
---@return number
-- Returns current (transformed) position
eng.mouse_position = function() end

---@return number
---@return number
-- Returns current (untransformed) position
eng.raw_mouse_position = function() end

---@return number
---@return number
-- Returns current window size (in pixels)
eng.window_size = function() end

---@return number
---@return number
-- Returns current viewport size (in pixels)
eng.size = function() end

---@param name string
---@param spatialize boolean?
---@return integer
---@see eng.sound_play
---@nodiscard
-- Loads a sound (only static for now...)
eng.load_sound = function(name, spatialize) end

---@param source integer
---@param settings AudioSettings?
---@return integer
---@see eng.load_sound
-- Plays a sound, and gives you a handle
eng.sound_play = function(source, settings) end

---@param sound integer
---@param gain number
-- Sets a sound's gain
eng.sound_gain = function(sound, gain) end

---@param sound integer
---@param pitch number
-- Sets a sound's pitch
eng.sound_pitch = function(sound, pitch) end

---@param sound integer
---@return
---| 'initial'
---| 'stopped'
---| 'playing'
---| 'paused'
-- Checks the state of the sound
eng.sound_state = function(sound) end

---@param position table
-- Sets the listener's position
eng.listener = function(position) end

---@param towards table
---@param up table
-- Sets the listener's orientation
eng.orientation = function(towards, up) end

---@param x number
---@param y number
---@param z number
---@return number
---@nodiscard
-- Simple perlin noise function (u cant set the seed lol)
eng.perlin = function(x, y, z) end

---@param name string
-- Sets the game's identity for savefiles
eng.identity = function(name) end

---@param data string
---@see eng.identity
-- Stores <data> into savefile
-- > NOTE: WON'T WORK WITHOUT HAVING SET THE IDENTITY FIRST!
eng.store = function(data) end

---@return string?
---@see eng.identity
-- Retrieves savefile data
-- > NOTE: WON'T WORK WITHOUT HAVING SET THE IDENTITY FIRST!
eng.retrieve = function() end

---@see eng.close
-- Schedules the program to exit next frame
eng.exit = function() end
