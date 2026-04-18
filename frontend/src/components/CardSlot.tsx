import { cardCodeToFilename } from '../data/cards'
import type { CardValue } from '../types/cards'

type CardSlotProps = {
  value: CardValue
  onClick: () => void
}

function getCardImage(value: CardValue) {
  if (value === null) return null

  if (value === 'BACK') {
    return new URL('../assets/cards/card-back.png', import.meta.url).href
  }

  return new URL(`../assets/cards/${cardCodeToFilename(value)}`, import.meta.url).href
}

export default function CardSlot({ value, onClick }: CardSlotProps) {
  const imageSrc = getCardImage(value)

  return (
    <button className="card-slot" onClick={onClick} type="button">
      {imageSrc ? (
        <img
          src={imageSrc}
          alt={value ?? 'Empty card slot'}
          className="card-image"
        />
      ) : (
        <div className="empty-card-slot">+</div>
      )}
    </button>
  )
}