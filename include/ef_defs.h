#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum Screen {
  SCREEN_MENU = 0,
  SCREEN_OPTIONS = 1,
  SCREEN_GAME = 2,
} Screen;

typedef enum Panel {
  PANEL_FORGE = 0,
  PANEL_MINE = 1,
  PANEL_UPGRADES = 2,
  PANEL_RECIPES = 3,
  PANEL_SHOP = 4,
  PANEL_PRESTIGE = 5,
  PANEL_COUNT = 6,
} Panel;

typedef enum SmeltSource {
  SMELT_SOURCE_MANUAL = 0,
  SMELT_SOURCE_AUTO = 1,
} SmeltSource;

typedef enum ResourceId {
  RES_STONE = 0,
  RES_COAL = 1,
  RES_COINS = 2,
  RES_IRON = 3,
  RES_COPPER = 4,
  RES_TIN = 5,
  RES_IRON_BAR = 6,
  RES_COPPER_BAR = 7,
  RES_BRONZE = 8,
  RES_GEAR = 9,
  RES_BELLOWS = 10,
  RES_STEEL = 11,
  RES_STEEL_PLATE = 12,
  RES_RIVET = 13,
  RES_MACHINE_PART = 14,
  RES_MYTHRIL = 15,
  RES_EMBER = 16,
  RES_COUNT = 17,
} ResourceId;

typedef enum BuildingId {
  BLD_PICKAXE = 0,
  BLD_COAL_PIT = 1,
  BLD_IRON_MINE = 2,
  BLD_COPPER_MINE = 3,
  BLD_TINSMITH = 4,
  BLD_FURNACE = 5,
  BLD_BELLOWS_SHOP = 6,
  BLD_MARKET_STALL = 7,
  BLD_MYTHRIL_DRILL = 8,
  BLD_COUNT = 9,
} BuildingId;

typedef enum RecipeId {
  RCP_IRON_BAR = 0,
  RCP_COPPER_BAR = 1,
  RCP_BRONZE = 2,
  RCP_STEEL = 3,
  RCP_GEAR = 4,
  RCP_BELLOWS_PART = 5,
  RCP_MACHINED_GEARS = 6,
  RCP_RIVETS = 7,
  RCP_STEEL_PLATE = 8,
  RCP_MACHINE_PART = 9,
  RCP_COIN_MINT = 10,
  RCP_EMBER_CRYSTAL = 11,
  RCP_COUNT = 12,
} RecipeId;

typedef enum UpgradeId {
  UPG_STURDY_GLOVES = 0,
  UPG_MINERS_POUCH = 1,
  UPG_QUICK_HANDS = 2,
  UPG_BETTER_PICKS = 3,
  UPG_COAL_VEINS = 4,
  UPG_STEEL_ALLOY = 5,
  UPG_SMELT_SLOT_1 = 6,
  UPG_SMELT_SLOT_2 = 7,
  UPG_SMELT_TUNING = 8,
  UPG_MARKET_NETWORK = 9,
  UPG_DEEP_DRILLING = 10,
  UPG_ANCIENT_VEINS = 11,
  UPG_REINFORCED_SACKS = 12,
  UPG_ORE_CRATES = 13,
  UPG_MARKET_SIGNAGE = 14,
  UPG_PRECISION_MOLDS = 15,
  UPG_METALWORKING = 16,
  UPG_COIN_MINTING = 17,
  UPG_INDUSTRIAL_TOOLS = 18,
  UPG_GEAR_DRIVEN_PICKS = 19,
  UPG_COAL_DUST_FILTERS = 20,
  UPG_ORE_SORTING = 21,
  UPG_BELLOWS_EFFICIENCY = 22,
  UPG_ASSEMBLY_LINES = 23,
  UPG_MECHANIZED_DRILLS = 24,
  UPG_COUNT = 25,
} UpgradeId;

typedef enum ShopItemId {
  SHOP_HAGGLING = 0,
  SHOP_SALESMANSHIP = 1,
  SHOP_WAREHOUSE = 2,
  SHOP_SMELTERS_COUPON = 3,
  SHOP_FOREMAN_CONTRACT = 4,
  SHOP_DEDICATED_CRUCIBLE = 5,
  SHOP_COUNT = 6,
} ShopItemId;

#define MAX_CRUCIBLES 4

typedef enum EternalUpgradeId {
  ETERNAL_MINER = 0,
  ETERNAL_EMBER_MEMORY = 1,
  ETERNAL_FORGE_MASTERY = 2,
  ETERNAL_ANCIENT_VEINS = 3,
  ETERNAL_TWIN_FURNACE = 4,
  ETERNAL_COUNT = 5,
} EternalUpgradeId;

typedef enum EventId {
  EVENT_NONE = -1,
  EVENT_CAVE_IN = 0,
  EVENT_WANDERING_MERCHANT = 1,
  EVENT_EMBER_PULSE = 2,
  EVENT_SCRAP_WINDFALL = 3,
  EVENT_ANCIENT_CACHE = 4,
  EVENT_COUNT = 5,
} EventId;

typedef enum AchievementId {
  ACH_FIRST_SPARK = 0,
  ACH_IRON_WILL = 1,
  ACH_THE_ALCHEMIST = 2,
  ACH_DEEP_ECHO = 3,
  ACH_EMBER_AWAKENED = 4,
  ACH_ETERNAL_FLAME = 5,
  ACH_STONE_STOCKPILE = 6,
  ACH_COAL_BARON = 7,
  ACH_PICKAXE_ARMY = 8,
  ACH_FURNACE_FLEET = 9,
  ACH_BELLOWS_BUSINESS = 10,
  ACH_MERCHANT_PRINCE = 11,
  ACH_GEARHEAD = 12,
  ACH_AIRFLOW_ARTISAN = 13,
  ACH_BRONZE_AGE = 14,
  ACH_STEELWORKS = 15,
  ACH_PLATE_PRESS = 16,
  ACH_RIVET_RAIN = 17,
  ACH_PARTS_BIN = 18,
  ACH_SHOPAHOLIC = 19,
  ACH_QUEUE_MASTER = 20,
  ACH_TINKERER = 21,
  ACH_INDUSTRIAL_REVOLUTION = 22,
  ACH_COUNT = 23,
} AchievementId;

const char *panel_key(Panel panel);
bool panel_from_key(const char *key, Panel *out_panel);

const char *smelt_source_key(SmeltSource source);
bool smelt_source_from_key(const char *key, SmeltSource *out_source);

const char *resource_key(ResourceId resource);
bool resource_from_key(const char *key, ResourceId *out_resource);

const char *building_key(BuildingId building);
bool building_from_key(const char *key, BuildingId *out_building);

const char *recipe_key(RecipeId recipe);
bool recipe_from_key(const char *key, RecipeId *out_recipe);

const char *upgrade_key(UpgradeId upgrade);
bool upgrade_from_key(const char *key, UpgradeId *out_upgrade);

const char *shop_item_key(ShopItemId item);
bool shop_item_from_key(const char *key, ShopItemId *out_item);

const char *eternal_upgrade_key(EternalUpgradeId up);
bool eternal_upgrade_from_key(const char *key, EternalUpgradeId *out_up);

const char *event_key(EventId event_id);
bool event_from_key(const char *key, EventId *out_event);

const char *achievement_key(AchievementId achievement);
bool achievement_from_key(const char *key, AchievementId *out_achievement);

