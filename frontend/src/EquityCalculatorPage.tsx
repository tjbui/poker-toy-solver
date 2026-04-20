import { useMemo, useState } from 'react'
import CardPickerModal from './CardPickerModal'
import CardRow from './CardRow'
import TableSection from './TableSection'
import type { CardCode, CardSelectionTarget, CardValue } from './cards'

export default function EquityCalculatorPage() {
  const [heroCards, setHeroCards] = useState<CardValue[]>(['BACK', 'BACK'])
  const [villainCards, setVillainCards] = useState<CardValue[]>(['BACK', 'BACK'])
  const [communityCards, setCommunityCards] = useState<CardValue[]>([
    'BACK',
    'BACK',
    'BACK',
    'BACK',
    'BACK',
  ])

  const [activeTarget, setActiveTarget] = useState<CardSelectionTarget | null>(null)

  const usedCards = useMemo(() => {
    return [...heroCards, ...villainCards, ...communityCards].filter(
      (card): card is CardCode => card !== null && card !== 'BACK',
    )
  }, [heroCards, villainCards, communityCards])

  function handleCardClick(section: CardSelectionTarget['section'], index: number) {
    setActiveTarget({ section, index })
  }

  function handleSelectCard(value: CardValue) {
    if (!activeTarget) return

    const { section, index } = activeTarget

    if (section === 'hero') {
      const next = [...heroCards]
      next[index] = value
      setHeroCards(next)
    } else if (section === 'villain') {
      const next = [...villainCards]
      next[index] = value
      setVillainCards(next)
    } else {
      const next = [...communityCards]
      next[index] = value
      setCommunityCards(next)
    }

    setActiveTarget(null)
  }

  return (
    <div className="page">
      <div className="table-shell">
        <h1 className="title">Poker Equity Calculator</h1>

        <div className="table-layout">
          <div className="top-row">
            <TableSection title="Hero Cards">
              <CardRow
                cards={heroCards}
                onCardClick={(index) => handleCardClick('hero', index)}
              />
            </TableSection>

            <TableSection title="Community Cards">
              <CardRow
                cards={communityCards}
                onCardClick={(index) => handleCardClick('community', index)}
              />
            </TableSection>
          </div>

          <div className="bottom-row">
            <TableSection title="Villain Cards">
              <CardRow
                cards={villainCards}
                onCardClick={(index) => handleCardClick('villain', index)}
              />
            </TableSection>
          </div>
        </div>
      </div>

      <CardPickerModal
        isOpen={activeTarget !== null}
        usedCards={usedCards}
        onClose={() => setActiveTarget(null)}
        onSelect={handleSelectCard}
      />
    </div>
  )
}